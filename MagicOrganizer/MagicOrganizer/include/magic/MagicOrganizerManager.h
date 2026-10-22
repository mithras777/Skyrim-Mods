#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace MITHRAS::MAGIC_ORGANIZER
{
	struct OrganizerConfig
	{
		bool enabled{ true };
		std::uint32_t hotkey{ 0x23 };  // H
	};

	struct SpellEntry
	{
		RE::FormID formID{ 0 };
		std::string displayName{};
		std::string stableFormID{};
	};

	struct PowerShoutEntry
	{
		RE::FormID formID{ 0 };
		std::string displayName{};
		std::string stableFormID{};
	};

	struct MagicEffectEntry
	{
		RE::FormID formID{ 0 };
		std::string displayName{};
		std::string stableFormID{};
	};

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		void Initialize();

		[[nodiscard]] OrganizerConfig GetConfig() const;
		void SetConfig(const OrganizerConfig& a_config, bool a_save);
		void SetEnabled(bool a_enabled);
		void SetHotkey(std::uint32_t a_hotkey);
		void Tick();

		void SetCaptureHotkey(bool a_capture);
		[[nodiscard]] bool IsCapturingHotkey() const;
		void OnHotkeyPressed(std::uint32_t a_keyCode);
		[[nodiscard]] static std::string GetKeyboardKeyName(std::uint32_t a_keyCode);

		[[nodiscard]] std::vector<SpellEntry> GetHiddenSpells() const;
		[[nodiscard]] std::vector<PowerShoutEntry> GetHiddenPowersAndShouts() const;
		[[nodiscard]] std::vector<MagicEffectEntry> GetHiddenEffects() const;

		bool HideSpell(RE::FormID a_formID);
		bool UnhideSpell(RE::FormID a_formID);
		bool UnhideSpell(std::string_view a_stableFormID);
		bool HidePowerOrShout(RE::FormID a_formID);
		bool UnhidePowerOrShout(RE::FormID a_formID);
		bool UnhidePowerOrShout(std::string_view a_stableFormID);
		bool HideEffect(RE::FormID a_formID);
		bool UnhideEffect(RE::FormID a_formID);
		bool UnhideEffect(std::string_view a_stableFormID);

		bool HideCurrentlySelectedMagicMenuEntry();
		[[nodiscard]] bool IsSpellHidden(RE::FormID a_formID) const;
		[[nodiscard]] bool IsPowerShoutHidden(RE::FormID a_formID) const;
		[[nodiscard]] bool IsEffectHidden(RE::FormID a_formID) const;

	private:
		struct StoredFormID
		{
			RE::FormID formID{ 0 };
			RE::FormID localFormID{ 0 };
			std::string pluginName{};
		};

		void SaveConfigToJson() const;
		void LoadConfigFromJson();
		[[nodiscard]] std::filesystem::path GetConfigPath() const;
		[[nodiscard]] static StoredFormID CreateStoredFormID(RE::FormID a_formID);

		void NormalizeConfig();
		void ApplyTrackedFlags();
		void RefreshMagicMenuIfOpen() const;

		[[nodiscard]] static bool IsFormIDHidden(const std::vector<StoredFormID>& a_hidden, RE::FormID a_formID);
		[[nodiscard]] static bool IsOrganizerSpell(const RE::SpellItem* a_spell);
		[[nodiscard]] static bool IsOrganizerPower(const RE::SpellItem* a_spell);
		[[nodiscard]] static bool IsOrganizerShout(const RE::TESShout* a_shout);

		[[nodiscard]] static std::string BuildSpellDisplayName(const RE::SpellItem* a_spell);
		[[nodiscard]] static std::string BuildPowerShoutDisplayName(const RE::SpellItem* a_spell);
		[[nodiscard]] static std::string BuildPowerShoutDisplayName(const RE::TESShout* a_shout);
		[[nodiscard]] static std::string BuildEffectDisplayName(const RE::EffectSetting* a_effect);

		[[nodiscard]] static std::vector<MagicEffectEntry> BuildPlayerActiveEffects();

		mutable std::mutex m_lock;
		OrganizerConfig m_config{};
		std::vector<StoredFormID> m_hiddenSpellFormIDs{};
		std::vector<StoredFormID> m_hiddenPowerShoutFormIDs{};
		std::vector<StoredFormID> m_hiddenEffectFormIDs{};
		bool m_captureHotkey{ false };
	};
}
