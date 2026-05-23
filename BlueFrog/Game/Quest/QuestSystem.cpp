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

QuestReward QuestSystem::TurnIn(const std::string& questId) noexcept
{
	auto it = state_.find(questId);
	if (it == state_.end()) return QuestReward{};
	if (it->second.status != QuestStatus::Complete) return QuestReward{};

	it->second.status = QuestStatus::TurnedIn;

	// The reward is owned by the registry's Quest, but we don't have
	// a registry pointer here. The caller (FLApp's turn-in dialog
	// handler) does the registry lookup and applies the reward —
	// keeping QuestSystem free of registry references. So this
	// function returns an empty reward; the caller fetches the real
	// one from registry.Find(questId)->reward.
	return QuestReward{};
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
