#include "ui/UI.h"

#include "magic/MagicOrganizerManager.h"
#include "util/LogUtil.h"

#include <SKSEMenuFramework.h>

#include <vector>

using namespace ImGuiMCP;

namespace UI
{
	namespace
	{
		struct KeyOption
		{
			std::uint32_t code{ 0 };
			std::string name{};
		};

		std::vector<KeyOption> BuildKeyboardOptions()
		{
			std::vector<KeyOption> out;
			out.reserve(256);
			for (std::uint32_t code = 1; code < 256; ++code) {
				const auto name = MITHRAS::MAGIC_ORGANIZER::Manager::GetKeyboardKeyName(code);
				if (name == "Unknown") {
					continue;
				}
				out.push_back({ code, name });
			}
			return out;
		}

		std::string StripDisplayFormID(std::string a_name)
		{
			if (a_name.size() >= 11 && a_name[a_name.size() - 1] == ']' && a_name[a_name.size() - 10] == '[' && a_name[a_name.size() - 11] == ' ') {
				a_name.erase(a_name.size() - 11);
			}
			return a_name;
		}

		template <class T, class Fn>
		void DrawHiddenTable(const char* a_tableID, const char* a_label, const std::vector<T>& a_entries, Fn&& a_unhide)
		{
			ImGuiMCP::TextColored(ImVec4(0.90f, 0.75f, 1.00f, 1.00f), "%s", a_label);
			ImGuiMCP::Separator();
			if (a_entries.empty()) {
				ImGuiMCP::TextDisabled("No hidden entries yet.");
				return;
			}

			std::vector<T> toUnhide;
			if (ImGuiMCP::BeginTable(a_tableID, 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
				ImGuiMCP::TableSetupColumn("Entry", ImGuiTableColumnFlags_WidthStretch);
				ImGuiMCP::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 34.0f);

				for (const auto& entry : a_entries) {
					if (!entry.stableFormID.empty()) {
						ImGuiMCP::PushID(entry.stableFormID.c_str());
					} else {
						ImGuiMCP::PushID(static_cast<int>(entry.formID));
					}
					ImGuiMCP::TableNextRow();
					ImGuiMCP::TableSetColumnIndex(0);
					const auto cleaned = StripDisplayFormID(entry.displayName);
					ImGuiMCP::TextUnformatted(cleaned.c_str());
					ImGuiMCP::TableSetColumnIndex(1);
					if (ImGuiMCP::Button("X")) {
						toUnhide.push_back(entry);
					}
					ImGuiMCP::PopID();
				}
				ImGuiMCP::EndTable();
			}

			for (const auto& entry : toUnhide) {
				a_unhide(entry);
			}
		}

		void DrawGeneralTab(MITHRAS::MAGIC_ORGANIZER::Manager* a_manager)
		{
			auto cfg = a_manager->GetConfig();
			const auto before = cfg;
			const auto keyOptions = BuildKeyboardOptions();

			ImGuiMCP::Checkbox("Enable", &cfg.enabled);

			const auto hotkeyName = MITHRAS::MAGIC_ORGANIZER::Manager::GetKeyboardKeyName(cfg.hotkey);
			if (ImGuiMCP::BeginCombo("Hotkey", hotkeyName.c_str())) {
				for (const auto& option : keyOptions) {
					const bool selected = (option.code == cfg.hotkey);
					if (ImGuiMCP::Selectable(option.name.c_str(), selected)) {
						cfg.hotkey = option.code;
					}
					if (selected) {
						ImGuiMCP::SetItemDefaultFocus();
					}
				}
				ImGuiMCP::EndCombo();
			}

			ImGuiMCP::Separator();
			ImGuiMCP::PushTextWrapPos(0.0f);
			ImGuiMCP::TextDisabled("Hidden entries use plugin-qualified FormIDs (e.g. 0x804~ExamplePlugin.esp), so load-order changes will not unhide them.");
			ImGuiMCP::TextColored(
				ImVec4(0.90f, 0.75f, 1.00f, 1.00f),
				"Tip: Open Magic Menu, highlight any spell/power/shout/effect, then press your hotkey to hide it. Hiding is UI-only.");
			ImGuiMCP::PopTextWrapPos();

			if (before.enabled != cfg.enabled) {
				a_manager->SetEnabled(cfg.enabled);
			}
			if (before.hotkey != cfg.hotkey) {
				a_manager->SetHotkey(cfg.hotkey);
			}
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			LOG_WARN("MagicOrganizer: SKSE Menu Framework not installed - menu unavailable");
			return;
		}

		SKSEMenuFramework::SetSection("Magic Organizer");
		SKSEMenuFramework::AddSectionItem("Settings", MainPanel::Render);
		LOG_INFO("MagicOrganizer: Registered SKSE Menu Framework section");
	}

	namespace MainPanel
	{
		void __stdcall Render()
		{
			auto* manager = MITHRAS::MAGIC_ORGANIZER::Manager::GetSingleton();
			if (ImGuiMCP::BeginTabBar("MagicOrganizerTabs")) {
				if (ImGuiMCP::BeginTabItem("General")) {
					DrawGeneralTab(manager);
					ImGuiMCP::EndTabItem();
				}

				if (ImGuiMCP::BeginTabItem("Spells")) {
					DrawHiddenTable("HiddenSpellsTable", "Hidden Spells", manager->GetHiddenSpells(), [&](const MITHRAS::MAGIC_ORGANIZER::SpellEntry& entry) {
						if (entry.stableFormID.empty()) {
							manager->UnhideSpell(entry.formID);
						} else {
							manager->UnhideSpell(entry.stableFormID);
						}
					});
					ImGuiMCP::EndTabItem();
				}

				if (ImGuiMCP::BeginTabItem("Powers")) {
					DrawHiddenTable("HiddenPowersTable", "Hidden Powers/Shouts", manager->GetHiddenPowersAndShouts(), [&](const MITHRAS::MAGIC_ORGANIZER::PowerShoutEntry& entry) {
						if (entry.stableFormID.empty()) {
							manager->UnhidePowerOrShout(entry.formID);
						} else {
							manager->UnhidePowerOrShout(entry.stableFormID);
						}
					});
					ImGuiMCP::EndTabItem();
				}

				if (ImGuiMCP::BeginTabItem("Magic Effects")) {
					DrawHiddenTable("HiddenEffectsTable", "Hidden Magic Effects", manager->GetHiddenEffects(), [&](const MITHRAS::MAGIC_ORGANIZER::MagicEffectEntry& entry) {
						if (entry.stableFormID.empty()) {
							manager->UnhideEffect(entry.formID);
						} else {
							manager->UnhideEffect(entry.stableFormID);
						}
					});
					ImGuiMCP::EndTabItem();
				}

				ImGuiMCP::EndTabBar();
			}
		}
	}
}
