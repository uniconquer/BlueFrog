#include "Window.h"

// Window Class Stuff
Window::WindowClass Window::WindowClass::wndClass;

const char* Window::WindowClass::GetName() noexcept
{
	return nullptr;
}

HINSTANCE Window::WindowClass::GetInstance() noexcept
{
	return HINSTANCE();
}

Window::WindowClass::WindowClass() noexcept
	:
	hInst(GetModuleHandle(nullptr))
{

}

Window::WindowClass::~WindowClass()
{
}

Window::Window(int width, int height, const char* name) noexcept
{
}

Window::~Window()
{
}
