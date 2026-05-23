#include "QuestSystem.h"

QuestStatus QuestSystem::Status(const std::string& questId) const noexcept
{
	auto it = state_.find(questId);
	return (it == state_.end()) ? QuestStatus::Available : it->second.status;
}

const std::vector<ObjectiveCondition>* QuestSystem::LiveConditions(const std::string& questId) const noexcept
{
	auto it = state_.find(questId);
	if (it == state_.end()) return nullptr;
	return &it->second.conditionsLive;
}

void QuestSystem::Accept(const std::string& questId, const QuestRegistry& registry) noexcept
{
	if (state_.count(questId) > 0)
	{
		// Already past Available — no-op (don't reset progress).
		return;
	}
	const Quest* def = registry.Find(questId);
	if (def == nullptr) return; // typo'd id — silently ignore

	Runtime rt;
	rt.status = QuestStatus::Active;
	rt.conditionsLive = def->conditions;  // copy with progress=0
	state_.emplace(questId, std::move(rt));
}

std::vector<QuestSystem::ConditionProgressEntry> QuestSystem::SnapshotState() const
{
	std::vector<ConditionProgressEntry> out;
	out.reserve(state_.size());
	for (const auto& [id, rt] : state_)
	{
		ConditionProgressEntry e;
		e.id     = id;
		e.status = static_cast<int>(rt.status);
		e.conditionProgress.reserve(rt.conditionsLive.size());
		for (const auto& cond : rt.conditionsLive)
		{
			// v1 has no OR groups in shipped quests, so each slot's
			// progress is just the single leaf's progress. When OR
			// groups arrive, this collapses to max(leaf progress)
			// per slot — meaningful, since hitting ANY leaf of an
			// OR group is what advances the slot.
			int slotProgress = 0;
			for (const auto& leaf : cond.leaves)
			{
				if (leaf.progress > slotProgress) slotProgress = leaf.progress;
			}
			e.conditionProgress.push_back(slotProgress);
		}
		out.push_back(std::move(e));
	}
	return out;
}

void QuestSystem::RestoreFromSnapshot(const std::vector<ConditionProgressEntry>& entries,
	const QuestRegistry& registry) noexcept
{
	state_.clear();
	for (const auto& e : entries)
	{
		const Quest* def = registry.Find(e.id);
		if (def == nullptr) continue; // stale save id — drop

		Runtime rt;
		rt.status         = static_cast<QuestStatus>(e.status);
		rt.conditionsLive = def->conditions; // start with definition's clean slate
		// Apply the saved per-slot progress. Spread it across the
		// slot's leaves (v1 has 1 leaf per slot, so this is
		// straightforward; with OR groups arriving later, restoring
		// "slot X had progress N" by setting every leaf's progress
		// to N is the simplest interpretation matching the
		// snapshot's "max across leaves" rule).
		const size_t n = std::min(rt.conditionsLive.size(), e.conditionProgress.size());
		for (size_t i = 0; i < n; ++i)
		{
			for (auto& leaf : rt.conditionsLive[i].leaves)
			{
				leaf.progress = e.conditionProgress[i];
			}
		}
		state_.emplace(e.id, std::move(rt));
	}
}

std::string QuestSystem::FindFirstInFlight() const noexcept
{
	for (const auto& [id, rt] : state_)
	{
		if (rt.status == QuestStatus::Active || rt.status == QuestStatus::Complete)
		{
			return id;
		}
	}
	return {};
}

bool QuestSystem::TurnIn(const std::string& questId) noexcept
{
	auto it = state_.find(questId);
	if (it == state_.end()) return false;
	if (it->second.status != QuestStatus::Complete) return false;
	it->second.status = QuestStatus::TurnedIn;
	return true;
}

void QuestSystem::Consume(const std::vector<GameEvent>& events) noexcept
{
	if (state_.empty()) return;

	for (auto& [id, rt] : state_)
	{
		if (rt.status != QuestStatus::Active) continue;

		for (const auto& e : events)
		{
			if (e.type != GameEventType::EnemyKilled) continue;

			for (auto& cond : rt.conditionsLive)
			{
				for (auto& leaf : cond.leaves)
				{
					if (leaf.type == "enemy_killed" && leaf.name == e.a && !leaf.IsMet())
					{
						leaf.progress += 1;
					}
				}
			}
		}

		// Recompute completion. Same AND-of-OR semantics as
		// ObjectiveState — all slots must have at least one met leaf.
		bool allMet = !rt.conditionsLive.empty();
		for (const auto& c : rt.conditionsLive)
		{
			if (!c.IsMet()) { allMet = false; break; }
		}
		if (allMet) rt.status = QuestStatus::Complete;
	}
}
