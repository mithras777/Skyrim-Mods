#include "magic/MagicOrganizerManager.h"

#include "util/LogUtil.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <format>
#include <fstream>
#include <limits>
#include <optional>
#include <set>

#include <nlohmann/json.hpp>

namespace MITHRAS::MAGIC_ORGANIZER
{
	namespace
	{
		using json = nlohmann::json;
		constexpr RE::EffectSetting::EffectSettingData::Flag kHideFlag = RE::EffectSetting::EffectSettingData::Flag::kHideInUI;
		constexpr auto kHiddenSpellsKey = "hidden_spells";
		constexpr auto kHiddenPowersKey = "hidden_powers";
		constexpr auto kHiddenEffectsKey = "hidden_effects";

		struct PluginFormID
		{
			RE::FormID localFormID{ 0 };
			std::string pluginName{};
		};

		struct LoadedFormIDs
		{
			std::vector<PluginFormID> pluginFormIDs{};
			std::vector<RE::FormID> legacyFormIDs{};
		};

		std::optional<PluginFormID> ParsePluginFormID(std::string_view a_value)
		{
			if (a_value.size() < 5 || (a_value[0] != '0') || (a_value[1] != 'x' && a_value[1] != 'X')) {
				return std::nullopt;
			}
			const auto separator = a_value.find('~', 2);
			if (separator == std::string_view::npos || separator == 2 || separator + 1 >= a_value.size()) {
				return std::nullopt;
			}

			std::uint32_t localFormID = 0;
			const auto* first = a_value.data() + 2;
			const auto* last = a_value.data() + separator;
			const auto parsed = std::from_chars(first, last, localFormID, 16);
			if (parsed.ec != std::errc{} || parsed.ptr != last || localFormID == 0 || localFormID > 0x00FFFFFF) {
				return std::nullopt;
			}

			return PluginFormID{ localFormID, std::string(a_value.substr(separator + 1)) };
		}

		std::string FormatPluginFormID(RE::FormID a_localFormID, std::string_view a_pluginName)
		{
			return std::format("0x{:X}~{}", a_localFormID, a_pluginName);
		}

		bool IsSamePluginFormID(RE::FormID a_leftLocalFormID, std::string_view a_leftPluginName, RE::FormID a_rightLocalFormID, std::string_view a_rightPluginName)
		{
			return a_leftLocalFormID == a_rightLocalFormID &&
			       a_leftPluginName.size() == a_rightPluginName.size() &&
			       _strnicmp(a_leftPluginName.data(), a_rightPluginName.data(), a_leftPluginName.size()) == 0;
		}

		std::optional<PluginFormID> GetPluginFormID(RE::FormID a_formID)
		{
			if (a_formID == 0 || a_formID >= 0xFF000000) {
				return std::nullopt;
			}
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler) {
				return std::nullopt;
			}

			const auto loadIndex = static_cast<std::uint8_t>(a_formID >> 24);
			const RE::TESFile* plugin = nullptr;
			RE::FormID localFormID = 0;
			if (loadIndex == 0xFE) {
				const auto lightIndex = static_cast<std::uint16_t>((a_formID & 0x00FFF000) >> 12);
				plugin = dataHandler->LookupLoadedLightModByIndex(lightIndex);
				localFormID = a_formID & 0x00000FFF;
			} else {
				plugin = dataHandler->LookupLoadedModByIndex(loadIndex);
				localFormID = a_formID & 0x00FFFFFF;
			}
			if (!plugin || plugin->GetFilename().empty() || localFormID == 0) {
				return std::nullopt;
			}
			return PluginFormID{ localFormID, std::string(plugin->GetFilename()) };
		}

		RE::TESForm* ResolvePluginFormID(const PluginFormID& a_pluginFormID)
		{
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			return dataHandler ? dataHandler->LookupForm(a_pluginFormID.localFormID, a_pluginFormID.pluginName) : nullptr;
		}

		LoadedFormIDs ReadFormIDArray(const json& a_root, const char* a_key)
		{
			LoadedFormIDs out;
			if (!a_root.contains(a_key) || !a_root[a_key].is_array()) {
				return out;
			}

			for (const auto& entry : a_root[a_key]) {
				if (entry.is_string()) {
					if (const auto pluginFormID = ParsePluginFormID(entry.get<std::string>()); pluginFormID.has_value()) {
						out.pluginFormIDs.push_back(*pluginFormID);
					}
					continue;
				}

				std::uint64_t raw = 0;
				if (entry.is_number_unsigned()) {
					raw = entry.get<std::uint64_t>();
				} else if (entry.is_number_integer()) {
					const auto signedRaw = entry.get<std::int64_t>();
					if (signedRaw <= 0) {
						continue;
					}
					raw = static_cast<std::uint64_t>(signedRaw);
				} else {
					continue;
				}
				if (raw <= std::numeric_limits<RE::FormID>::max()) {
					const auto formID = static_cast<RE::FormID>(raw);
					if (formID != 0) {
						out.legacyFormIDs.push_back(formID);
					}
				}
			}
			return out;
		}

		std::optional<RE::FormID> ResolveSelectedFormIDFromMovieView(const RE::MagicItemList* a_itemList)
		{
			if (!a_itemList || !a_itemList->view) {
				return std::nullopt;
			}
			RE::GFxValue val{};
			if (!a_itemList->view->GetVariable(&val, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId") || !val.IsNumber()) {
				return std::nullopt;
			}
			const auto raw = static_cast<std::uint64_t>(val.GetNumber());
			const auto formID = static_cast<RE::FormID>(raw & 0xFFFFFFFFu);
			return formID ? std::optional<RE::FormID>(formID) : std::nullopt;
		}

		RE::MagicItemList::Item* ResolveSelectedItemFallback(RE::MagicItemList* a_itemList)
		{
			if (!a_itemList) {
				return nullptr;
			}
			const auto count = a_itemList->items.size();
			if (count == 0) {
				return nullptr;
			}

			constexpr std::array<std::string_view, 5> kIndexKeys{ "selectedIndex", "_selectedIndex", "itemIndex", "index", "selectedEntry" };
			RE::GFxValue idxValue{};
			for (const auto key : kIndexKeys) {
				if (a_itemList->entryList.GetMember(key.data(), &idxValue) && idxValue.IsNumber()) {
					const auto index = static_cast<std::int32_t>(idxValue.GetSInt());
					if (index >= 0 && static_cast<std::size_t>(index) < count) {
						return a_itemList->items[static_cast<std::size_t>(index)];
					}
				}
			}
			for (const auto key : kIndexKeys) {
				if (a_itemList->root.GetMember(key.data(), &idxValue) && idxValue.IsNumber()) {
					const auto index = static_cast<std::int32_t>(idxValue.GetSInt());
					if (index >= 0 && static_cast<std::size_t>(index) < count) {
						return a_itemList->items[static_cast<std::size_t>(index)];
					}
				}
			}
			return nullptr;
		}

		std::optional<RE::FormID> ResolveSelectedActiveEffectFormID(RE::MagicItemList::Item* a_selected)
		{
			if (!a_selected) {
				return std::nullopt;
			}

			std::vector<RE::FormID> activeBaseFormIDs;
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* target = player ? player->GetMagicTarget() : nullptr;
			auto* activeList = target ? target->GetActiveEffectList() : nullptr;
			if (!activeList) {
				return std::nullopt;
			}

			std::set<RE::FormID> uniqueIDs;
			for (auto* activeEffect : *activeList) {
				if (!activeEffect) {
					continue;
				}
				auto* effect = activeEffect->GetBaseObject();
				if (!effect || effect->IsDeleted()) {
					continue;
				}
				const auto formID = effect->GetFormID();
				if (formID == 0 || uniqueIDs.contains(formID)) {
					continue;
				}
				uniqueIDs.insert(formID);
				activeBaseFormIDs.push_back(formID);
			}

			if (activeBaseFormIDs.empty()) {
				return std::nullopt;
			}

			auto isActive = [&](RE::FormID a_formID) {
				for (const auto formID : activeBaseFormIDs) {
					if (formID == a_formID) {
						return true;
					}
				}
				return false;
			};
			if (auto* selectedEffect = a_selected->data.baseForm ? a_selected->data.baseForm->As<RE::EffectSetting>() : nullptr) {
				const auto selectedFormID = selectedEffect->GetFormID();
				if (selectedFormID != 0 && isActive(selectedFormID)) {
					return selectedFormID;
				}
			}
			if (const auto* selectedMagicItem = a_selected->data.baseForm ? a_selected->data.baseForm->As<RE::MagicItem>() : nullptr) {
				for (const auto* itemEffect : selectedMagicItem->effects) {
					if (!itemEffect || !itemEffect->baseEffect) {
						continue;
					}
					const auto formID = itemEffect->baseEffect->GetFormID();
					if (formID != 0 && isActive(formID)) {
						return formID;
					}
				}
			}
			const char* selectedName = a_selected->data.GetName();
			if (!selectedName || selectedName[0] == '\0') {
				return std::nullopt;
			}
			for (auto* activeEffect : *activeList) {
				if (!activeEffect) {
					continue;
				}
				auto* base = activeEffect->GetBaseObject();
				if (!base) {
					continue;
				}
				const char* baseName = base->GetName();
				if (baseName && _stricmp(baseName, selectedName) == 0) {
					return base->GetFormID();
				}
			}
			return std::nullopt;
		}

	}

	void Manager::Initialize()
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = {};
			m_captureHotkey = false;
		}
		LoadConfigFromJson();
		ApplyTrackedFlags();
	}

	OrganizerConfig Manager::GetConfig() const
	{
		std::scoped_lock lock(m_lock);
		return m_config;
	}

	void Manager::SetConfig(const OrganizerConfig& a_config, bool a_save)
	{
		{
			std::scoped_lock lock(m_lock);
			m_config = a_config;
			NormalizeConfig();
		}
		ApplyTrackedFlags();
		if (a_save) {
			SaveConfigToJson();
		}
	}

	void Manager::SetEnabled(bool a_enabled)
	{
		auto cfg = GetConfig();
		cfg.enabled = a_enabled;
		SetConfig(cfg, true);
	}

	void Manager::SetHotkey(std::uint32_t a_hotkey)
	{
		auto cfg = GetConfig();
		cfg.hotkey = a_hotkey;
		SetConfig(cfg, true);
	}

	void Manager::Tick()
	{
		// Filter-only mode has no periodic reconciliation work.
	}

	void Manager::SetCaptureHotkey(bool a_capture)
	{
		std::scoped_lock lock(m_lock);
		m_captureHotkey = a_capture;
	}

	bool Manager::IsCapturingHotkey() const
	{
		std::scoped_lock lock(m_lock);
		return m_captureHotkey;
	}

	void Manager::OnHotkeyPressed(std::uint32_t a_keyCode)
	{
		bool update = false;
		{
			std::scoped_lock lock(m_lock);
			if (!m_captureHotkey) {
				return;
			}
			m_captureHotkey = false;
			m_config.hotkey = a_keyCode;
			update = true;
		}
		if (update) {
			LOG_INFO("MagicOrganizer: Hotkey updated to {}", a_keyCode);
			SaveConfigToJson();
		}
	}

	std::string Manager::GetKeyboardKeyName(std::uint32_t a_keyCode)
	{
		auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
		if (!inputMgr) {
			return "Unknown";
		}
		RE::BSFixedString name{};
		if (inputMgr->GetButtonNameFromID(RE::INPUT_DEVICE::kKeyboard, static_cast<std::int32_t>(a_keyCode), name)) {
			if (const auto* c = name.c_str(); c && c[0] != '\0') {
				return c;
			}
		}
		return std::format("Key {}", a_keyCode);
	}

	std::vector<SpellEntry> Manager::GetHiddenSpells() const
	{
		std::vector<StoredFormID> hiddenFormIDs;
		{
			std::scoped_lock lock(m_lock);
			hiddenFormIDs = m_hiddenSpellFormIDs;
		}
		std::vector<SpellEntry> hidden;
		hidden.reserve(hiddenFormIDs.size());
		for (const auto& storedFormID : hiddenFormIDs) {
			auto* spell = storedFormID.formID ? RE::TESForm::LookupByID<RE::SpellItem>(storedFormID.formID) : nullptr;
			const auto stableFormID = storedFormID.pluginName.empty() ? std::string{} : FormatPluginFormID(storedFormID.localFormID, storedFormID.pluginName);
			if (spell && IsOrganizerSpell(spell)) {
				hidden.push_back({ storedFormID.formID, BuildSpellDisplayName(spell), stableFormID });
			} else {
				const auto missingID = stableFormID.empty() ? std::format("{:08X}", storedFormID.formID) : stableFormID;
				hidden.push_back({ storedFormID.formID, std::format("<Missing {}>", missingID), stableFormID });
			}
		}
		return hidden;
	}

	std::vector<PowerShoutEntry> Manager::GetHiddenPowersAndShouts() const
	{
		std::vector<StoredFormID> hiddenFormIDs;
		{
			std::scoped_lock lock(m_lock);
			hiddenFormIDs = m_hiddenPowerShoutFormIDs;
		}
		std::vector<PowerShoutEntry> hidden;
		hidden.reserve(hiddenFormIDs.size());
		for (const auto& storedFormID : hiddenFormIDs) {
			auto* spell = storedFormID.formID ? RE::TESForm::LookupByID<RE::SpellItem>(storedFormID.formID) : nullptr;
			const auto stableFormID = storedFormID.pluginName.empty() ? std::string{} : FormatPluginFormID(storedFormID.localFormID, storedFormID.pluginName);
			if (spell && IsOrganizerPower(spell)) {
				hidden.push_back({ storedFormID.formID, BuildPowerShoutDisplayName(spell), stableFormID });
				continue;
			}
			auto* shout = storedFormID.formID ? RE::TESForm::LookupByID<RE::TESShout>(storedFormID.formID) : nullptr;
			if (shout && IsOrganizerShout(shout)) {
				hidden.push_back({ storedFormID.formID, BuildPowerShoutDisplayName(shout), stableFormID });
			} else {
				const auto missingID = stableFormID.empty() ? std::format("{:08X}", storedFormID.formID) : stableFormID;
				hidden.push_back({ storedFormID.formID, std::format("<Missing {}>", missingID), stableFormID });
			}
		}
		return hidden;
	}

	std::vector<MagicEffectEntry> Manager::GetHiddenEffects() const
	{
		std::vector<StoredFormID> hiddenFormIDs;
		{
			std::scoped_lock lock(m_lock);
			hiddenFormIDs = m_hiddenEffectFormIDs;
		}
		std::vector<MagicEffectEntry> hidden;
		hidden.reserve(hiddenFormIDs.size());
		for (const auto& storedFormID : hiddenFormIDs) {
			auto* effect = storedFormID.formID ? RE::TESForm::LookupByID<RE::EffectSetting>(storedFormID.formID) : nullptr;
			const auto stableFormID = storedFormID.pluginName.empty() ? std::string{} : FormatPluginFormID(storedFormID.localFormID, storedFormID.pluginName);
			if (effect) {
				hidden.push_back({ storedFormID.formID, BuildEffectDisplayName(effect), stableFormID });
			} else {
				const auto missingID = stableFormID.empty() ? std::format("{:08X}", storedFormID.formID) : stableFormID;
				hidden.push_back({ storedFormID.formID, std::format("<Missing {}>", missingID), stableFormID });
			}
		}
		return hidden;
	}

	Manager::StoredFormID Manager::CreateStoredFormID(RE::FormID a_formID)
	{
		StoredFormID storedFormID{};
		storedFormID.formID = a_formID;
		if (const auto pluginFormID = GetPluginFormID(a_formID); pluginFormID.has_value()) {
			storedFormID.localFormID = pluginFormID->localFormID;
			storedFormID.pluginName = pluginFormID->pluginName;
		}
		return storedFormID;
	}

	bool Manager::HideSpell(RE::FormID a_formID)
	{
		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(a_formID);
		if (!spell || !IsOrganizerSpell(spell)) {
			return false;
		}
		bool changed = false;
		{
			std::scoped_lock lock(m_lock);
			if (!IsFormIDHidden(m_hiddenSpellFormIDs, a_formID)) {
				m_hiddenSpellFormIDs.push_back(CreateStoredFormID(a_formID));
				NormalizeConfig();
				changed = true;
			}
		}
		if (!changed) {
			return false;
		}
		SaveConfigToJson();
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::UnhideSpell(RE::FormID a_formID)
	{
		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_hiddenSpellFormIDs;
			auto it = std::remove_if(hidden.begin(), hidden.end(), [a_formID](const StoredFormID& a_entry) { return a_entry.formID == a_formID; });
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
		}
		if (!removed) {
			return false;
		}
		SaveConfigToJson();
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::UnhideSpell(std::string_view a_stableFormID)
	{
		const auto pluginFormID = ParsePluginFormID(a_stableFormID);
		if (!pluginFormID) {
			return false;
		}
		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_hiddenSpellFormIDs;
			auto it = std::remove_if(hidden.begin(), hidden.end(), [&](const StoredFormID& a_entry) {
				return !a_entry.pluginName.empty() && IsSamePluginFormID(a_entry.localFormID, a_entry.pluginName, pluginFormID->localFormID, pluginFormID->pluginName);
			});
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
		}
		if (!removed) {
			return false;
		}
		SaveConfigToJson();
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::HidePowerOrShout(RE::FormID a_formID)
	{
		auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(a_formID);
		auto* shout = RE::TESForm::LookupByID<RE::TESShout>(a_formID);
		const bool isPower = spell && IsOrganizerPower(spell);
		const bool isShout = shout && IsOrganizerShout(shout);
		if (!isPower && !isShout) {
			return false;
		}
		bool changed = false;
		{
			std::scoped_lock lock(m_lock);
			if (!IsFormIDHidden(m_hiddenPowerShoutFormIDs, a_formID)) {
				m_hiddenPowerShoutFormIDs.push_back(CreateStoredFormID(a_formID));
				NormalizeConfig();
				changed = true;
			}
		}
		if (!changed) {
			return false;
		}
		SaveConfigToJson();
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::UnhidePowerOrShout(RE::FormID a_formID)
	{
		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_hiddenPowerShoutFormIDs;
			auto it = std::remove_if(hidden.begin(), hidden.end(), [a_formID](const StoredFormID& a_entry) { return a_entry.formID == a_formID; });
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
		}
		if (!removed) {
			return false;
		}
		SaveConfigToJson();
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::UnhidePowerOrShout(std::string_view a_stableFormID)
	{
		const auto pluginFormID = ParsePluginFormID(a_stableFormID);
		if (!pluginFormID) {
			return false;
		}
		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_hiddenPowerShoutFormIDs;
			auto it = std::remove_if(hidden.begin(), hidden.end(), [&](const StoredFormID& a_entry) {
				return !a_entry.pluginName.empty() && IsSamePluginFormID(a_entry.localFormID, a_entry.pluginName, pluginFormID->localFormID, pluginFormID->pluginName);
			});
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
		}
		if (!removed) {
			return false;
		}
		SaveConfigToJson();
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::HideEffect(RE::FormID a_formID)
	{
		if (a_formID == 0) {
			return false;
		}
		bool changed = false;
		{
			std::scoped_lock lock(m_lock);
			if (!IsFormIDHidden(m_hiddenEffectFormIDs, a_formID)) {
				m_hiddenEffectFormIDs.push_back(CreateStoredFormID(a_formID));
				NormalizeConfig();
				changed = true;
			}
		}
		if (!changed) {
			return false;
		}
		SaveConfigToJson();
		ApplyTrackedFlags();
		return true;
	}

	bool Manager::UnhideEffect(RE::FormID a_formID)
	{
		bool removed = false;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_hiddenEffectFormIDs;
			auto it = std::remove_if(hidden.begin(), hidden.end(), [a_formID](const StoredFormID& a_entry) { return a_entry.formID == a_formID; });
			if (it != hidden.end()) {
				hidden.erase(it, hidden.end());
				removed = true;
			}
		}
		if (!removed) {
			return false;
		}
		if (auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(a_formID)) {
			effect->data.flags.reset(kHideFlag);
		}
		SaveConfigToJson();
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::UnhideEffect(std::string_view a_stableFormID)
	{
		const auto pluginFormID = ParsePluginFormID(a_stableFormID);
		if (!pluginFormID) {
			return false;
		}
		std::vector<RE::FormID> removedFormIDs;
		{
			std::scoped_lock lock(m_lock);
			auto& hidden = m_hiddenEffectFormIDs;
			auto it = std::remove_if(hidden.begin(), hidden.end(), [&](const StoredFormID& a_entry) {
				const bool matches = !a_entry.pluginName.empty() && IsSamePluginFormID(a_entry.localFormID, a_entry.pluginName, pluginFormID->localFormID, pluginFormID->pluginName);
				if (matches && a_entry.formID != 0) {
					removedFormIDs.push_back(a_entry.formID);
				}
				return matches;
			});
			if (it == hidden.end()) {
				return false;
			}
			hidden.erase(it, hidden.end());
		}
		for (const auto formID : removedFormIDs) {
			if (auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(formID)) {
				effect->data.flags.reset(kHideFlag);
			}
		}
		SaveConfigToJson();
		RefreshMagicMenuIfOpen();
		return true;
	}

	bool Manager::HideCurrentlySelectedMagicMenuEntry()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
			LOG_INFO("MagicOrganizer: Hide request ignored (MagicMenu not open)");
			return false;
		}
		auto magicMenu = ui->GetMenu<RE::MagicMenu>();
		if (!magicMenu) {
			return false;
		}
		auto& runtime = magicMenu->GetRuntimeData();
		if (!runtime.itemList) {
			return false;
		}

		auto* selected = runtime.itemList->GetSelectedItem();
		if (!selected) {
			selected = ResolveSelectedItemFallback(runtime.itemList);
		}

		if (selected) {
			if (auto* shout = selected->data.baseForm ? selected->data.baseForm->As<RE::TESShout>() : nullptr) {
				return HidePowerOrShout(shout->GetFormID());
			}
			if (auto* spell = selected->data.baseForm ? selected->data.baseForm->As<RE::SpellItem>() : nullptr) {
				if (IsOrganizerPower(spell)) {
					return HidePowerOrShout(spell->GetFormID());
				}
				if (IsOrganizerSpell(spell)) {
					return HideSpell(spell->GetFormID());
				}
			}
			if (const auto effectFormID = ResolveSelectedActiveEffectFormID(selected); effectFormID.has_value()) {
				return HideEffect(*effectFormID);
			}
		}

		if (const auto fallbackFormID = ResolveSelectedFormIDFromMovieView(runtime.itemList); fallbackFormID.has_value()) {
			if (auto* shout = RE::TESForm::LookupByID<RE::TESShout>(*fallbackFormID)) {
				(void)shout;
				return HidePowerOrShout(*fallbackFormID);
			}
			if (auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(*fallbackFormID)) {
				if (IsOrganizerPower(spell)) {
					return HidePowerOrShout(*fallbackFormID);
				}
				if (IsOrganizerSpell(spell)) {
					return HideSpell(*fallbackFormID);
				}
			}
		}

		LOG_INFO("MagicOrganizer: Hotkey pressed but no hideable selected entry was resolved");
		return false;
	}

	bool Manager::IsSpellHidden(RE::FormID a_formID) const
	{
		if (a_formID == 0) {
			return false;
		}
		std::scoped_lock lock(m_lock);
		return IsFormIDHidden(m_hiddenSpellFormIDs, a_formID);
	}

	bool Manager::IsPowerShoutHidden(RE::FormID a_formID) const
	{
		if (a_formID == 0) {
			return false;
		}
		std::scoped_lock lock(m_lock);
		return IsFormIDHidden(m_hiddenPowerShoutFormIDs, a_formID);
	}

	bool Manager::IsEffectHidden(RE::FormID a_formID) const
	{
		if (a_formID == 0) {
			return false;
		}
		std::scoped_lock lock(m_lock);
		return IsFormIDHidden(m_hiddenEffectFormIDs, a_formID);
	}

	void Manager::SaveConfigToJson() const
	{
		const auto path = GetConfigPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		OrganizerConfig cfg{};
		std::vector<StoredFormID> hiddenSpells;
		std::vector<StoredFormID> hiddenPowers;
		std::vector<StoredFormID> hiddenEffects;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			hiddenSpells = m_hiddenSpellFormIDs;
			hiddenPowers = m_hiddenPowerShoutFormIDs;
			hiddenEffects = m_hiddenEffectFormIDs;
		}
		auto writeFormIDArray = [](const auto& a_formIDs) {
			json out = json::array();
			for (const auto& formID : a_formIDs) {
				if (!formID.pluginName.empty() && formID.localFormID != 0) {
					out.push_back(FormatPluginFormID(formID.localFormID, formID.pluginName));
				} else if (formID.formID != 0) {
					out.push_back(formID.formID);
				}
			}
			return out;
		};
		json root = {
			{ "enabled", cfg.enabled },
			{ "hotkey", cfg.hotkey },
			{ kHiddenSpellsKey, writeFormIDArray(hiddenSpells) },
			{ kHiddenPowersKey, writeFormIDArray(hiddenPowers) },
			{ kHiddenEffectsKey, writeFormIDArray(hiddenEffects) }
		};
		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			LOG_WARN("MagicOrganizer: Failed to write config {}", path.string());
			return;
		}
		file << root.dump(2);
	}

	void Manager::LoadConfigFromJson()
	{
		const auto path = GetConfigPath();
		if (!std::filesystem::exists(path)) {
			SaveConfigToJson();
			return;
		}
		std::ifstream file(path);
		if (!file.is_open()) {
			LOG_WARN("MagicOrganizer: Failed to read config {}", path.string());
			return;
		}
		json parsed{};
		try {
			file >> parsed;
		} catch (const std::exception& e) {
			LOG_WARN("MagicOrganizer: Config parse failed ({}), rewriting defaults", e.what());
			SaveConfigToJson();
			return;
		}
		OrganizerConfig cfg{};
		LoadedFormIDs loadedSpells;
		LoadedFormIDs loadedPowers;
		LoadedFormIDs loadedEffects;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
		}
		cfg.enabled = parsed.value("enabled", cfg.enabled);
		cfg.hotkey = parsed.value("hotkey", cfg.hotkey);
		loadedSpells = ReadFormIDArray(parsed, kHiddenSpellsKey);
		loadedPowers = ReadFormIDArray(parsed, kHiddenPowersKey);
		loadedEffects = ReadFormIDArray(parsed, kHiddenEffectsKey);

		bool migratedLegacyIDs = false;
		auto buildStoredFormIDs = [&](const LoadedFormIDs& a_loaded, auto a_isValidForm) {
			std::vector<StoredFormID> storedFormIDs;
			storedFormIDs.reserve(a_loaded.pluginFormIDs.size() + a_loaded.legacyFormIDs.size());
			for (const auto& pluginFormID : a_loaded.pluginFormIDs) {
				StoredFormID storedFormID{};
				storedFormID.localFormID = pluginFormID.localFormID;
				storedFormID.pluginName = pluginFormID.pluginName;
				if (auto* form = ResolvePluginFormID(pluginFormID); form && a_isValidForm(form)) {
					storedFormID.formID = form->GetFormID();
				}
				storedFormIDs.push_back(std::move(storedFormID));
			}
			for (const auto formID : a_loaded.legacyFormIDs) {
				auto* form = RE::TESForm::LookupByID(formID);
				if (form && a_isValidForm(form)) {
					auto storedFormID = CreateStoredFormID(form->GetFormID());
					if (!storedFormID.pluginName.empty()) {
						storedFormIDs.push_back(std::move(storedFormID));
						migratedLegacyIDs = true;
						continue;
					}
				}
				storedFormIDs.push_back({ formID, 0, {} });
			}
			return storedFormIDs;
		};
		auto isSpell = [](RE::TESForm* a_form) {
			auto* spell = a_form ? a_form->As<RE::SpellItem>() : nullptr;
			return spell && !IsOrganizerPower(spell);
		};
		auto isPowerOrShout = [](RE::TESForm* a_form) {
			auto* spell = a_form ? a_form->As<RE::SpellItem>() : nullptr;
			return (spell && IsOrganizerPower(spell)) || (a_form && a_form->As<RE::TESShout>());
		};
		auto isEffect = [](RE::TESForm* a_form) { return a_form && a_form->As<RE::EffectSetting>(); };
		auto storedSpells = buildStoredFormIDs(loadedSpells, isSpell);
		auto storedPowers = buildStoredFormIDs(loadedPowers, isPowerOrShout);
		auto storedEffects = buildStoredFormIDs(loadedEffects, isEffect);
		{
			std::scoped_lock lock(m_lock);
			m_config = cfg;
			m_hiddenSpellFormIDs = std::move(storedSpells);
			m_hiddenPowerShoutFormIDs = std::move(storedPowers);
			m_hiddenEffectFormIDs = std::move(storedEffects);
			NormalizeConfig();
		}
		if (migratedLegacyIDs) {
			SaveConfigToJson();
		}
	}

	std::filesystem::path Manager::GetConfigPath() const
	{
		wchar_t dllPath[MAX_PATH]{};
		GetModuleFileNameW(GetModuleHandleW(L"MagicOrganizer.dll"), dllPath, MAX_PATH);
		return std::filesystem::path(dllPath).parent_path() / "MagicOrganizer.json";
	}

	void Manager::NormalizeConfig()
	{
		auto normalize = [](std::vector<StoredFormID>& a_list) {
			a_list.erase(std::remove_if(a_list.begin(), a_list.end(), [](const StoredFormID& a_id) {
				return a_id.pluginName.empty() ? a_id.formID == 0 : a_id.localFormID == 0;
			}), a_list.end());
			std::sort(a_list.begin(), a_list.end(), [](const StoredFormID& a_left, const StoredFormID& a_right) {
				const bool leftStable = !a_left.pluginName.empty();
				const bool rightStable = !a_right.pluginName.empty();
				if (leftStable != rightStable) {
					return leftStable;
				}
				if (leftStable) {
					const auto pluginOrder = _stricmp(a_left.pluginName.c_str(), a_right.pluginName.c_str());
					return pluginOrder == 0 ? a_left.localFormID < a_right.localFormID : pluginOrder < 0;
				}
				return a_left.formID < a_right.formID;
			});
			a_list.erase(std::unique(a_list.begin(), a_list.end(), [](const StoredFormID& a_left, const StoredFormID& a_right) {
				if (!a_left.pluginName.empty() && !a_right.pluginName.empty()) {
					return IsSamePluginFormID(a_left.localFormID, a_left.pluginName, a_right.localFormID, a_right.pluginName);
				}
				return a_left.pluginName.empty() && a_right.pluginName.empty() && a_left.formID == a_right.formID;
			}), a_list.end());
		};
		normalize(m_hiddenSpellFormIDs);
		normalize(m_hiddenPowerShoutFormIDs);
		normalize(m_hiddenEffectFormIDs);
	}

	void Manager::ApplyTrackedFlags()
	{
		std::vector<StoredFormID> hiddenEffects;
		{
			std::scoped_lock lock(m_lock);
			hiddenEffects = m_hiddenEffectFormIDs;
		}

		for (const auto& storedFormID : hiddenEffects) {
			if (storedFormID.formID != 0) {
				if (auto* effect = RE::TESForm::LookupByID<RE::EffectSetting>(storedFormID.formID)) {
					effect->data.flags.reset(kHideFlag);
				}
			}
		}
		RefreshMagicMenuIfOpen();
	}

	void Manager::RefreshMagicMenuIfOpen() const
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen(RE::MagicMenu::MENU_NAME)) {
			return;
		}
		auto magicMenu = ui->GetMenu<RE::MagicMenu>();
		if (!magicMenu) {
			return;
		}
		auto& runtime = magicMenu->GetRuntimeData();
		if (!runtime.itemList) {
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		runtime.itemList->Update(player);

		OrganizerConfig cfg{};
		std::vector<StoredFormID> hiddenSpellFormIDs;
		std::vector<StoredFormID> hiddenPowerFormIDs;
		std::vector<StoredFormID> hiddenEffectFormIDs;
		{
			std::scoped_lock lock(m_lock);
			cfg = m_config;
			hiddenSpellFormIDs = m_hiddenSpellFormIDs;
			hiddenPowerFormIDs = m_hiddenPowerShoutFormIDs;
			hiddenEffectFormIDs = m_hiddenEffectFormIDs;
		}
		if (!cfg.enabled || (hiddenSpellFormIDs.empty() && hiddenPowerFormIDs.empty() && hiddenEffectFormIDs.empty())) {
			return;
		}

		auto& items = runtime.itemList->items;
		for (std::uint32_t i = items.size(); i > 0; --i) {
			const auto idx = i - 1;
			const auto* item = items[idx];
			if (!item || !item->data.baseForm) {
				continue;
			}
			const auto* baseForm = item->data.baseForm;
			const auto formID = baseForm->GetFormID();
			bool shouldHide = false;
			if (const auto* effect = baseForm->As<RE::EffectSetting>()) {
				shouldHide = IsFormIDHidden(hiddenEffectFormIDs, effect->GetFormID());
			} else if (const auto* spell = baseForm->As<RE::SpellItem>()) {
				shouldHide = IsOrganizerPower(spell) ? IsFormIDHidden(hiddenPowerFormIDs, formID) : IsFormIDHidden(hiddenSpellFormIDs, formID);
			} else if (baseForm->As<RE::TESShout>()) {
				shouldHide = IsFormIDHidden(hiddenPowerFormIDs, formID);
			}
			if (shouldHide) {
				items.erase(items.begin() + idx);
			}
		}

		auto& entryList = runtime.itemList->entryList;
		const auto arraySize = entryList.GetArraySize();
		for (std::uint32_t i = arraySize; i > 0; --i) {
			const auto idx = i - 1;
			RE::GFxValue entry{};
			if (!entryList.GetElement(idx, &entry) || !entry.IsObject()) {
				continue;
			}
			RE::GFxValue formIdVal{};
			if (!entry.GetMember("formId", &formIdVal) || !formIdVal.IsNumber()) {
				continue;
			}
			const auto raw = static_cast<std::uint64_t>(formIdVal.GetNumber());
			const auto formID = static_cast<RE::FormID>(raw & 0xFFFFFFFFu);
			if (formID == 0) {
				continue;
			}
			const bool hiddenSpell = IsFormIDHidden(hiddenSpellFormIDs, formID);
			const bool hiddenPower = IsFormIDHidden(hiddenPowerFormIDs, formID);
			const bool hiddenEffect = IsFormIDHidden(hiddenEffectFormIDs, formID);
			if (hiddenSpell || hiddenPower || hiddenEffect) {
				entryList.RemoveElements(idx, 1);
			}
		}
	}

	bool Manager::IsFormIDHidden(const std::vector<StoredFormID>& a_hidden, RE::FormID a_formID)
	{
		return std::find_if(a_hidden.begin(), a_hidden.end(), [a_formID](const StoredFormID& a_entry) {
			return a_entry.formID == a_formID;
		}) != a_hidden.end();
	}

	bool Manager::IsOrganizerSpell(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return false;
		}
		const auto type = a_spell->GetSpellType();
		return type != RE::MagicSystem::SpellType::kPower &&
		       type != RE::MagicSystem::SpellType::kLesserPower &&
		       type != RE::MagicSystem::SpellType::kVoicePower;
	}

	bool Manager::IsOrganizerPower(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return false;
		}
		const auto type = a_spell->GetSpellType();
		return type == RE::MagicSystem::SpellType::kPower ||
		       type == RE::MagicSystem::SpellType::kLesserPower ||
		       type == RE::MagicSystem::SpellType::kVoicePower;
	}

	bool Manager::IsOrganizerShout(const RE::TESShout* a_shout)
	{
		return a_shout != nullptr;
	}

	std::string Manager::BuildSpellDisplayName(const RE::SpellItem* a_spell)
	{
		if (!a_spell) {
			return "<None>";
		}
		const char* name = a_spell->GetName();
		if (name && name[0] != '\0') {
			return std::format("{} [{:08X}]", name, a_spell->GetFormID());
		}
		const char* editorID = a_spell->GetFormEditorID();
		if (editorID && editorID[0] != '\0') {
			return std::format("{} [{:08X}]", editorID, a_spell->GetFormID());
		}
		return std::format("<{:08X}>", a_spell->GetFormID());
	}

	std::string Manager::BuildPowerShoutDisplayName(const RE::SpellItem* a_spell)
	{
		return BuildSpellDisplayName(a_spell);
	}

	std::string Manager::BuildPowerShoutDisplayName(const RE::TESShout* a_shout)
	{
		if (!a_shout) {
			return "<None>";
		}
		const char* name = a_shout->GetName();
		if (name && name[0] != '\0') {
			return std::format("{} [{:08X}]", name, a_shout->GetFormID());
		}
		const char* editorID = a_shout->GetFormEditorID();
		if (editorID && editorID[0] != '\0') {
			return std::format("{} [{:08X}]", editorID, a_shout->GetFormID());
		}
		return std::format("<{:08X}>", a_shout->GetFormID());
	}

	std::string Manager::BuildEffectDisplayName(const RE::EffectSetting* a_effect)
	{
		if (!a_effect) {
			return "<None>";
		}
		const char* name = a_effect->GetName();
		if (name && name[0] != '\0') {
			return std::format("{} [{:08X}]", name, a_effect->GetFormID());
		}
		const char* editorID = a_effect->GetFormEditorID();
		if (editorID && editorID[0] != '\0') {
			return std::format("{} [{:08X}]", editorID, a_effect->GetFormID());
		}
		return std::format("<{:08X}>", a_effect->GetFormID());
	}

	std::vector<MagicEffectEntry> Manager::BuildPlayerActiveEffects()
	{
		std::vector<MagicEffectEntry> out;
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* target = player ? player->GetMagicTarget() : nullptr;
		auto* activeList = target ? target->GetActiveEffectList() : nullptr;
		if (!activeList) {
			return out;
		}

		std::set<RE::FormID> uniqueIDs;
		for (auto* activeEffect : *activeList) {
			if (!activeEffect) {
				continue;
			}
			auto* effect = activeEffect->GetBaseObject();
			if (!effect || effect->IsDeleted()) {
				continue;
			}
			const auto formID = effect->GetFormID();
			if (formID == 0 || uniqueIDs.contains(formID)) {
				continue;
			}
			uniqueIDs.insert(formID);
			out.push_back({ formID, BuildEffectDisplayName(effect) });
		}

		std::sort(out.begin(), out.end(), [](const MagicEffectEntry& a_lhs, const MagicEffectEntry& a_rhs) {
			return _stricmp(a_lhs.displayName.c_str(), a_rhs.displayName.c_str()) < 0;
		});
		return out;
	}
}
