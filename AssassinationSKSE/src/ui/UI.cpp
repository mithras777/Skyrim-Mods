#include "ui/UI.h"

#include "game/Assassination.h"
#include "util/LogUtil.h"

#include <SKSEMenuFramework.h>

using namespace ImGuiMCP;

namespace UI {
	namespace {
		[[nodiscard]] bool ConfigChanged(const GAME::ASSASSINATION::Config& a_lhs, const GAME::ASSASSINATION::Config& a_rhs)
		{
			return a_lhs.enabled != a_rhs.enabled ||
			       a_lhs.maxLevelDifference != a_rhs.maxLevelDifference ||
			       a_lhs.assassinationChance != a_rhs.assassinationChance ||
			       a_lhs.melee != a_rhs.melee ||
			       a_lhs.unarmed != a_rhs.unarmed ||
			       a_lhs.bowsCrossbows != a_rhs.bowsCrossbows ||
			       a_lhs.excludeBosses != a_rhs.excludeBosses ||
			       a_lhs.excludeDragons != a_rhs.excludeDragons ||
			       a_lhs.excludeGiants != a_rhs.excludeGiants;
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("AssassinationSKSE: SKSE Menu Framework not installed - MCM unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Assassination SKSE");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("AssassinationSKSE: registered MCM panel");
	}

	namespace MainPanel {
		void __stdcall Render()
		{
			auto cfg = GAME::ASSASSINATION::GetConfig();
			const auto before = cfg;

			if (ImGuiMCP::BeginTabBar("AssassinationTabs")) {
				if (ImGuiMCP::BeginTabItem("General")) {
					if (ImGuiMCP::CollapsingHeader("Assassination", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGuiMCP::Checkbox("Enabled", &cfg.enabled);
						ImGuiMCP::TextDisabled("When disabled, the hit listener remains active but will never execute targets.");
						ImGuiMCP::SliderInt("Max Level Difference", &cfg.maxLevelDifference, 0, 100);
						ImGuiMCP::TextDisabled("0 = target must be your level or lower. 100 = target can be up to 100 levels above you.");
						ImGuiMCP::SliderInt("Assassination Chance", &cfg.assassinationChance, 0, 100);
						ImGuiMCP::TextDisabled("100 = always assassinate. 0 = never assassinate.");
					}
					ImGuiMCP::EndTabItem();
				}

				if (ImGuiMCP::BeginTabItem("Attack Types")) {
					if (ImGuiMCP::CollapsingHeader("Allowed attacks", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGuiMCP::Checkbox("Melee", &cfg.melee);
						ImGuiMCP::Checkbox("Unarmed", &cfg.unarmed);
						ImGuiMCP::Checkbox("Bows / Crossbows", &cfg.bowsCrossbows);
					}
					ImGuiMCP::EndTabItem();
				}

				if (ImGuiMCP::BeginTabItem("Exclusions")) {
					if (ImGuiMCP::CollapsingHeader("Target filters", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGuiMCP::Checkbox("Exclude bosses", &cfg.excludeBosses);
						ImGuiMCP::TextDisabled("Uses a best-effort boss heuristic based on unique, essential, protected, or boss-tagged targets.");
						ImGuiMCP::Checkbox("Exclude dragons", &cfg.excludeDragons);
						ImGuiMCP::TextDisabled("Matches dragon actors/races through ActorTypeDragon keywords.");
						ImGuiMCP::Checkbox("Exclude giants", &cfg.excludeGiants);
						ImGuiMCP::TextDisabled("Matches giant actors/races through ActorTypeGiant keywords.");
					}
					ImGuiMCP::EndTabItem();
				}

				ImGuiMCP::EndTabBar();
			}

			if (ConfigChanged(before, cfg)) {
				GAME::ASSASSINATION::SetConfig(cfg);
			}
		}
	}
}
