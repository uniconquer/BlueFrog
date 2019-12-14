#include "App.h"
#include <sstream>
#include <iomanip>

App::App()
	:
	wnd(800, 600, "Blue Frog")
{
}

int App::Go()
{
	MSG msg;
	BOOL gResult;
	while ((gResult = GetMessage(&msg, nullptr, 0, 0)) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		DoFrame();
	}

	if (gResult == -1)
	{
		throw BFWND_LAST_EXCEPT();
	}

	return (int)msg.wParam;
}

void App::DoFrame()
{
	const float t = timer.Peek();
	std::ostringstream oss;
	oss << "Time elapsed : " << std::setprecision(1) << std::fixed << t << "s";
	wnd.SetTitle(oss.str());
}
