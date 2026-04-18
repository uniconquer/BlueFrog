#include "App.h"
#include "../Engine/Scene/PrefabLoader.h"
#include "../Engine/Scene/SceneLoader.h"

#include <shellapi.h>
#include <filesystem>
#include <string>
#include <system_error>

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

	// Dry-run every shipped scene and prefab before the window is created.
	// Catches typos, stray commas, and missing prefab references at startup
	// instead of at scene-transition time. A missing directory is not fatal
	// (fresh checkouts or partial installs may not have every folder), but
	// any file that exists must parse cleanly.
	bool ValidateAllAssets(std::string* errorOut)
	{
		auto sweep = [&](const std::filesystem::path& dir, const std::string& suffix, auto validator) -> bool
		{
			std::error_code ec;
			if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec))
			{
				return true; // nothing to validate
			}
			for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
			{
				if (ec) break;
				if (!entry.is_regular_file()) continue;
				const std::string name = entry.path().filename().string();
				if (name.size() < suffix.size()) continue;
				if (name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) continue;
				if (!validator(entry.path(), errorOut))
				{
					return false;
				}
			}
			return true;
		};

		if (!sweep(std::filesystem::path("Assets/Scenes"), ".json",
			[](const std::filesystem::path& p, std::string* e) { return SceneLoader::Validate(p, e); }))
		{
			return false;
		}
		if (!sweep(std::filesystem::path("Assets/Prefabs"), ".prefab.json",
			[](const std::filesystem::path& p, std::string* e) { return PrefabLoader::Validate(p, e); }))
		{
			return false;
		}
		return true;
	}
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	{
		std::string validationError;
		if (!ValidateAllAssets(&validationError))
		{
			MessageBoxA(nullptr, validationError.c_str(), "Asset Validation Failed", MB_OK | MB_ICONEXCLAMATION);
			return -1;
		}
	}

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
