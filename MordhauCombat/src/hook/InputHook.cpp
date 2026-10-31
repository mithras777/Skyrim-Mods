#include "hook/InputHook.h"

#include "combat/DirectionalController.h"

#include "ui/VectorOverlay.h"

namespace HOOK::INPUT {




	struct PlayerUpdate {
		static void thunk(RE::PlayerCharacter* a_this, float a_delta)
		{
			OnPlayerUpdate(a_delta);
			func(a_this, a_delta);
		}

		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		// Install the mod player update hook.
		
		stl::Hook::virtual_function<RE::PlayerCharacter, PlayerUpdate>(0, 0xAD);
		MC::UI::VectorOverlay::Install();
	}

	void OnPlayerUpdate(float a_delta)
	{
		MC::DIRECTIONAL::Controller::GetSingleton()->Update(a_delta);
		MC::UI::VectorOverlay::Update(a_delta);
	}
}
