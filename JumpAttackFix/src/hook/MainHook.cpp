#include "hook/MainHook.h"

#include "hook/InputHook.h"

#include "util/LogUtil.h"

namespace HOOK::MAIN {

	void Install()
	{
		LOG_INFO("Hooks initializing...");

		HOOK::INPUT::Install();
	}
}
