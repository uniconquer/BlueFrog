#include "AppBase.h"

#include <algorithm>

AppBase::AppBase(int windowWidth, int windowHeight, std::wstring windowTitle)
	:
	wnd(windowWidth, windowHeight, windowTitle.c_str())
{
}

int AppBase::Run()
{
	OnStartup();
	while (true)
	{
		if (const auto ecode = Window::ProcessMessages())
		{
			// WM_QUIT path — give the game one last hook before we
			// propagate the exit code. OnShutdown is noexcept so any
			// teardown failure can't suppress the quit.
			OnShutdown();
			return *ecode;
		}

		// Clamp the frame delta: scene loads (mip generation, mesh import)
		// stall the loop for seconds, and feeding that spike into one tick
		// made the AI sprint half the arena, land hits, and shove the player
		// through a wall before the first visible frame. 100ms keeps slow
		// frames playable while turning load hitches into a brief slow-mo
		// instead of a physics explosion.
		constexpr float kMaxFrameDt = 0.1f;
		const float dt = (std::min)(timer.Mark(), kMaxFrameDt);
		OnUpdate(dt);
		OnRender();
	}
}
