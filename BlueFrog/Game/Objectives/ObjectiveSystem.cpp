#include "ObjectiveSystem.h"

bool ObjectiveState::IsComplete() const noexcept
{
	for (const auto& c : conditions)
	{
		if (!c.IsMet())
		{
			return false;
		}
	}
	return true;
}

void ObjectiveSystem::Reset(ObjectiveState state) noexcept
{
	state_ = std::move(state);
}

void ObjectiveSystem::Consume(const std::vector<GameEvent>& events) noexcept
{
	for (const auto& e : events)
	{
		if (e.type != GameEventType::EnemyKilled)
		{
			continue;
		}
		// A single event may match multiple leaves across multiple conditions
		// (e.g. one OR group asks for EnemyScout, another counts toward
		// "any enemy"). Walk all of them so each independent counter advances.
		for (auto& cond : state_.conditions)
		{
			for (auto& leaf : cond.leaves)
			{
				if (leaf.type == "enemy_killed" && leaf.name == e.a)
				{
					if (leaf.progress < leaf.required)
					{
						++leaf.progress;
					}
				}
			}
		}
	}
}

std::wstring ObjectiveSystem::CurrentText() const noexcept
{
	return state_.IsComplete() ? state_.completionText : state_.text;
}
