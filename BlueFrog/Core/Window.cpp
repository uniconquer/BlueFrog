#include "Window.h"

// Window Class Stuff
Window::WindowClass Window::WindowClass::wndClass;

const char* Window::WindowClass::GetName() noexcept
{
	return wndClassName;
}

HINSTANCE Window::WindowClass::GetInstance() noexcept
{
	return wndClass.hInst;
}

Window::WindowClass::WindowClass() noexcept
	:
	hInst(GetModuleHandle(nullptr))
{
	// ������ Ŭ���� ���
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetInstance();
	wc.hIcon = nullptr;
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = GetName();
	wc.hIconSm = nullptr;
	RegisterClassEx(&wc);
}

Window::WindowClass::~WindowClass()
{
	UnregisterClass(wndClassName, GetInstance());
}

Window::Window(int width, int height, const char* name) noexcept
	:
	width(width),
	height(height),
	hWnd(nullptr)
{
	RECT wr;
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;
	AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);

	// ������ ���� �� hWnd�� ����
	HWND hWnd = CreateWindow(WindowClass::GetName(), name,
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
		nullptr, nullptr, WindowClass::GetInstance(), this);

	// show window
	ShowWindow(hWnd, SW_SHOWDEFAULT);

}

Window::~Window()
{
	DestroyWindow(hWnd);
}

LRESULT CALLBACK Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// CreateWindow()���� ���޵� create �Ű� ������ ����Ͽ� WinAPI�� ������ Ŭ���� �����͸� ����
	if (msg == WM_NCCREATE)
	{
		// ������ �����Ϳ��� ������ Ŭ������ �����͸� ����
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		// �����͸� ������ �ν��Ͻ��� �����ϵ��� WinAPI�� �����ϴ� ���� ������ ����
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		// set message proc to normal (non-setup) handler now that setup is finished
		// ������ �Ϸ�Ǿ����Ƿ� �޽��� ���ν����� �Ϲ�(����) �ڵ鷯�� ����
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk));
		// ������ �ν��Ͻ� �ڵ鷯�� �޽����� ����
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}
	// WM_NCCREATE �޽��� ������ �޽����� ǥ�õǸ� �⺻ �ڵ鷯�� ó��
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK Window::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// �����͸� ������ �ν��Ͻ��� �˻�
	Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	// ������ �ν��Ͻ� �ڵ鷯�� �޽����� ����
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
