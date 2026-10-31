#include "hook/InputHook.h"

#include "console/ConsoleCommands.h"
#include "diagonal/FakeDiagonalSprint.h"

namespace HOOK::INPUT {

	static constexpr auto CONSOLE_PREFIX = "test";

	struct ProcessConsole {

		static void thunk(RE::Script* a_script, RE::ScriptCompiler* a_compiler, RE::COMPILER_NAME a_name, RE::TESObjectREFR* a_objectRefr)
		{
			const std::string text = a_script->text;
			if (text.starts_with(CONSOLE_PREFIX)) {
				CONSOLE::COMMANDS::Plugin(a_script);
				return;
			}

			func(a_script, a_compiler, a_name, a_objectRefr);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct PlayerControlsInput {
		static RE::BSEventNotifyControl thunk(RE::PlayerControls* a_this, RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_source)
		{
			DIAGONAL::FakeDiagonalSprint::GetSingleton()->OnInputEvent(a_event);
			return func(a_this, a_event, a_source);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct PlayerUpdate {
		static void thunk(RE::PlayerCharacter* a_this, float a_delta)
		{
			if (a_this && a_this->IsPlayerRef()) {
				DIAGONAL::FakeDiagonalSprint::GetSingleton()->OnPlayerUpdate(a_delta);
			}
			func(a_this, a_delta);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		DIAGONAL::FakeDiagonalSprint::GetSingleton()->Initialize();

		const auto runtime = REL::Module::get().version();
        if (runtime == REL::Version(1, 5, 97, 0) || runtime == REL::Version(1, 6, 1170, 0)) {
            const auto site = RELOCATION_ID(52065, 52952).address() + RELOCATION_OFFSET(0xE2, 0x52);
            if (*reinterpret_cast<const std::uint8_t*>(site) == 0xE8) {
                stl::Hook::call<ProcessConsole>(site, "DiagonalSprint console reload");
            } else {
                LOG_WARN("Console reload hook skipped: expected CALL instruction");
            }
        } else {
            LOG_WARN("Console reload hook skipped on {}: offsets are unverified", runtime.string());
        }
		stl::Hook::virtual_function<RE::PlayerControls, PlayerControlsInput>(0, 0x1);
		stl::Hook::virtual_function<RE::PlayerCharacter, PlayerUpdate>(0, 0xAD);
	}
}
