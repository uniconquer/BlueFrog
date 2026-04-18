#include "App.h"

#include <shellapi.h>
#include <string>

namespace
{
	// Minimal CLI parser: recognizes `--scene <path>`. Unknown tokens are ignored.
	// The asset tree uses ASCII-only paths, so we narrow wchar_t -> char directly;
	// if non-ASCII scene paths are ever needed, swap this for a proper UTF-8
	// conversion (the project defines NONLS, which hides WideCharToMultiByte).
	std::string ExtractScenePathFromCommandLine()
	{
		int argc = 0;
		LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
		if (argv == nullptr)
		{
			return {};
		}

		std::string scenePath;
		for (int i = 1; i + 1 < argc; ++i)
		{
			if (std::wstring(argv[i]) == L"--scene")
			{
				for (const wchar_t* p = argv[i + 1]; *p != L'\0'; ++p)
				{
					scenePath.push_back(static_cast<char>(*p));
				}
				break;
			}
		}

		::LocalFree(argv);
		return scenePath;
	}
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	try
	{
		return App{ ExtractScenePathFromCommandLine() }.Go();
	}
	catch (const BFException& e)
	{
		MessageBoxA(nullptr, e.what(), e.GetType(), MB_OK | MB_ICONEXCLAMATION);
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		MessageBox(nullptr, L"No details available", L"Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	return -1;
}
