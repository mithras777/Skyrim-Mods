#include "hook/InputHook.h"

#include "console/ConsoleCommands.h"

namespace HOOK::INPUT {

	static constexpr auto CONSOLE_PREFIX = "test";

	struct ProcessConsole {

		static void thunk(RE::Script* a_script, RE::ScriptCompiler* a_compiler, RE::COMPILER_NAME a_name, RE::TESObjectREFR* a_objectRefr)
		{
			const std::string text = a_script->text;
			// Search for specific prefix.
			if (text.starts_with(CONSOLE_PREFIX)) {
				CONSOLE::COMMANDS::Plugin(a_script);
				return;
			}

			func(a_script, a_compiler, a_name, a_objectRefr);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		// Keep the legacy demo available without patching unverified 1.7.x offsets.
#if defined(MOD_ENABLE_DEMO_CONSOLE_HOOK)
		const auto runtime = REL::Module::get().version();
		if (runtime != REL::Version(1, 5, 97, 0) && runtime != REL::Version(1, 6, 1170, 0)) {
			LOG_WARN("Legacy console demo skipped on Skyrim {}: its offsets are unverified", runtime.string());
			return;
		}
		const auto site = RELOCATION_ID(52065, 52952).address() + RELOCATION_OFFSET(0xE2, 0x52);
		if (*reinterpret_cast<const std::uint8_t*>(site) != 0xE8) {
			LOG_WARN("Legacy console demo skipped: expected a CALL instruction at the patch site");
			return;
		}
		stl::Hook::call<ProcessConsole>(RELOCATION_ID(52065, 52952).address() + RELOCATION_OFFSET(0xE2, 0x52), "ProcessConsole");
#else
		LOG_INFO("Basic hook entry point ready; the legacy console demo is disabled (unverified 1.7.x offsets)");
#endif
	}
}
