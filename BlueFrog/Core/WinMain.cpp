#include "App.h"

std::wstring str2wstr(const char* string) noexcept;

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	try
	{
		return App{}.Go();
	}
	catch (const BFException& e)
	{
		MessageBox(nullptr, str2wstr(e.what()).c_str(), str2wstr(e.GetType()).c_str(), MB_OK | MB_ICONEXCLAMATION);
	}
	catch (const std::exception& e)
	{
		MessageBox(nullptr, str2wstr(e.what()).c_str(), L"Standard Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		MessageBox(nullptr, L"No details available", L"Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	return -1;
}

std::wstring str2wstr(const char* string) noexcept
{
	std::string str(string);
	std::wstring wstr(str.size(), ' ');
	std::copy(str.begin(), str.end(), wstr.begin());
	return wstr;
}