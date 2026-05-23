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

			out.quests.clear();
			if (j.contains("quests") && j["quests"].is_array())
			{
				for (const auto& q : j["quests"])
				{
					QuestStateSnapshot s;
					s.id     = q.value("id", std::string{});
					s.status = q.value("status", 0);
					if (q.contains("conditionProgress") && q["conditionProgress"].is_array())
					{
						for (const auto& p : q["conditionProgress"])
						{
							s.conditionProgress.push_back(p.get<int>());
						}
					}
					if (!s.id.empty()) out.quests.push_back(std::move(s));
				}
			}

			out.inventory.clear();
			if (j.contains("inventory") && j["inventory"].is_array())
			{
				for (const auto& it : j["inventory"])
				{
					InventoryEntrySnapshot e;
					e.id    = it.value("id", std::string{});
					e.count = it.value("count", 0);
					if (!e.id.empty() && e.count > 0) out.inventory.push_back(std::move(e));
				}
			}

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

			json quests = json::array();
			for (const auto& q : profile.quests)
			{
				json qj;
				qj["id"]     = q.id;
				qj["status"] = q.status;
				json prog = json::array();
				for (int p : q.conditionProgress) prog.push_back(p);
				qj["conditionProgress"] = std::move(prog);
				quests.push_back(std::move(qj));
			}
			j["quests"] = std::move(quests);

			json inv = json::array();
			for (const auto& e : profile.inventory)
			{
				json ej;
				ej["id"]    = e.id;
				ej["count"] = e.count;
				inv.push_back(std::move(ej));
			}
			j["inventory"] = std::move(inv);

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
