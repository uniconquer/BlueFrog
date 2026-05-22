#include "AppBase.h"

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

		const float dt = timer.Mark();
		OnUpdate(dt);
		OnRender();
	}
}
