#include "PlayerProfile.h"

#include "../../Core/BFWin.h"
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

using json = nlohmann::json;

namespace
{
	void Log(const std::string& msg)
	{
		::OutputDebugStringA(msg.c_str());
		::OutputDebugStringA("\n");
	}
}

namespace PlayerProfileIO
{
	bool Load(const std::filesystem::path& path, PlayerProfile& out) noexcept
	{
		try
		{
			std::ifstream f(path);
			if (!f.is_open()) return false;
			json j;
			f >> j;

			out.scenePath       = j.value("scenePath", std::string{});
			out.playerHealth    = j.value("playerHealth", 0);
			out.playerMaxHealth = j.value("playerMaxHealth", 0);
			out.playTimeSec     = j.value("playTimeSec", 0.0f);

			if (out.scenePath.empty()) return false;
			return true;
		}
		catch (const std::exception& e)
		{
			Log(std::string("[PlayerProfile] Load failed: ") + e.what());
			return false;
		}
	}

	bool Save(const std::filesystem::path& path, const PlayerProfile& profile) noexcept
	{
		try
		{
			std::error_code ec;
			std::filesystem::create_directories(path.parent_path(), ec);

			json j;
			j["scenePath"]       = profile.scenePath;
			j["playerHealth"]    = profile.playerHealth;
			j["playerMaxHealth"] = profile.playerMaxHealth;
			j["playTimeSec"]     = profile.playTimeSec;

			std::ofstream f(path);
			if (!f.is_open())
			{
				Log("[PlayerProfile] Save failed: cannot open file for write");
				return false;
			}
			f << j.dump(2);
			f << '\n';
			return true;
		}
		catch (const std::exception& e)
		{
			Log(std::string("[PlayerProfile] Save failed: ") + e.what());
			return false;
		}
	}
}
