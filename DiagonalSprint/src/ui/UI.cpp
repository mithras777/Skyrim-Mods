#include "ui/UI.h"

#include "diagonal/FakeDiagonalSprint.h"
#include "util/LogUtil.h"

using namespace ImGuiMCP;

namespace UI
{
	namespace
	{
		void ResetGeneralDefaults(DIAGONAL::FakeDiagonalConfig& a_cfg)
		{
			const DIAGONAL::FakeDiagonalConfig defaults{};
			a_cfg.enabled = defaults.enabled;
		}

		void ResetControlsDefaults(DIAGONAL::FakeDiagonalConfig& a_cfg)
		{
			const DIAGONAL::FakeDiagonalConfig defaults{};
			a_cfg.lateralSpeed = defaults.lateralSpeed;
			a_cfg.freeAirControl = defaults.freeAirControl;
			a_cfg.controllerDiagonalConeDegrees = defaults.controllerDiagonalConeDegrees;
		}

		bool ConfigChanged(const DIAGONAL::FakeDiagonalConfig& a_lhs, const DIAGONAL::FakeDiagonalConfig& a_rhs)
		{
			return a_lhs.enabled != a_rhs.enabled ||
			       a_lhs.lateralSpeed != a_rhs.lateralSpeed ||
			       a_lhs.freeAirControl != a_rhs.freeAirControl ||
			       a_lhs.controllerDiagonalConeDegrees != a_rhs.controllerDiagonalConeDegrees;
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("DiagonalSprint: SKSE Menu Framework not installed - MCM unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Diagonal Sprint");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("DiagonalSprint: registered MCM panel");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* manager = DIAGONAL::FakeDiagonalSprint::GetSingleton();
			auto cfg = manager->GetConfig();
			const auto before = cfg;

			if (ImGuiMCP::BeginTabBar("DiagonalSprintTabs")) {
				if (ImGuiMCP::BeginTabItem("General")) {
					if (ImGuiMCP::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGuiMCP::Checkbox("Enabled", &cfg.enabled);
						ImGuiMCP::Spacing();
						if (ImGuiMCP::Button("Defaults##General")) {
							ResetGeneralDefaults(cfg);
						}
					}
					ImGuiMCP::EndTabItem();
				}

				if (ImGuiMCP::BeginTabItem("Controls")) {
					if (ImGuiMCP::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGuiMCP::SliderFloat("Lateral Speed", &cfg.lateralSpeed, 0.5f, 10.0f, "%.1f");
						ImGuiMCP::Checkbox("Free Air Control", &cfg.freeAirControl);
						ImGuiMCP::SliderFloat("Controller Diagonal Cone", &cfg.controllerDiagonalConeDegrees, 15.0f, 85.0f, "%.0f deg");
						ImGuiMCP::TextDisabled("Higher values accept a wider stick angle; default is very forgiving.");
						ImGuiMCP::Spacing();
						if (ImGuiMCP::Button("Defaults##Controls")) {
							ResetControlsDefaults(cfg);
						}
					}
					ImGuiMCP::EndTabItem();
				}

				ImGuiMCP::EndTabBar();
			}

			if (ConfigChanged(before, cfg)) {
				manager->SetConfig(cfg, true);
			}
		}
	}
}
