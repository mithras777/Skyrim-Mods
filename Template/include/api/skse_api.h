#pragma once

#include <SKSE/SKSE.h>

#define SKSE_API extern "C" __declspec(dllexport)

namespace SKSE {
	SKSE_API bool SKSEPlugin_Query(const QueryInterface* a_skse, PluginInfo* a_pluginInfo);
	SKSE_API bool SKSEPlugin_Load(const LoadInterface* a_skse);
}
