#include "hook/InputHook.h"


#include "overhaul/SpellbladeOverhaulManager.h"

namespace HOOK::INPUT {




	struct PlayerUpdate {
		static void thunk(RE::PlayerCharacter* a_this, float a_delta)
		{
			SB_OVERHAUL::Manager::GetSingleton()->Update(a_this, a_delta);
			func(a_this, a_delta);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		// Install the mod player update hook.
		
		stl::Hook::virtual_function<RE::PlayerCharacter, PlayerUpdate>(0, 0xAD);
	}
}
