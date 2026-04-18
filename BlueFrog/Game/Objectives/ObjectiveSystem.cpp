#include "ObjectiveSystem.h"

bool ObjectiveState::IsComplete() const noexcept
{
	for (const auto& c : conditions)
	{
		if (!c.met)
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
		for (auto& c : state_.conditions)
		{
			if (c.type == "enemy_killed" && c.name == e.a)
			{
				c.met = true;
			}
		}
	}
}

std::wstring ObjectiveSystem::CurrentText() const noexcept
{
	return state_.IsComplete() ? state_.completionText : state_.text;
}
