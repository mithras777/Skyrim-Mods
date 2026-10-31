#include "api/skse_api.h"

#include "plugin.h"
#include "version.h"
#include "event/AttackAnimationEventSink.h"
#include "event/EquipEventSink.h"
#include "event/GameEventManager.h"
#include "event/mastery/MasteryEventSinks.h"
#include "input/InputEventSink.h"
#include "overhaul/SpellbladeOverhaulManager.h"
#include "serialization/Serialization.h"
#include "ui/PrismaBridge.h"
#include "util/LogUtil.h"

//#define DUMP_OFFSETS

#if defined(DUMP_OFFSETS)
#include "api/versionlibdb.h"

static void DumpOffsets(REL::Version a_gameVersion)
{
	VersionDb db;
	// Try to load database for current running executable version.
	if (!db.Load(a_gameVersion.major(), a_gameVersion.minor(), a_gameVersion.patch(), a_gameVersion.build())) {
		LOG_CRITICAL("Failed to load database for {}!", a_gameVersion.string("."));
		return;
	}

	// Dump offsets file to My Games\Skyrim Special Edition\SKSE directory
	auto dumpOffsets = std::format("{}\\{}-offsets.txt", SKSE::log::log_directory()->string(), a_gameVersion.string("."));
	db.Dump(dumpOffsets);
	LOG_INFO("Dumped offsets for SkyrimSE v{}", a_gameVersion.string("."));
}
#endif

SKSEPluginInfo(
    .Version = REL::Version{ VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD },
    .Name = VERSION_PRODUCTNAME_STR,
    .Author = VERSION_AUTHOR_STR,
    .StructCompatibility = SKSE::StructCompatibility::Dependent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary
);

namespace SKSE {

	static void SKSEPlugin_Message(SKSE::MessagingInterface::Message* a_message)
	{
		switch (a_message->type) {

			case SKSE::MessagingInterface::kPostPostLoad: {
#if defined(SKSE_SUPPORT_XBYAK)
				// Install Hook(s)
				::stl::Hook::install();
#endif
				break;
			}
			case SKSE::MessagingInterface::kInputLoaded: {
				SB_INPUT::EventSink::Register();
				break;
			}
			case SKSE::MessagingInterface::kDataLoaded: {
				SB_OVERHAUL::Manager::GetSingleton()->Initialize();
				SB_INPUT::EventSink::Register();
				// Register game events
				GAME_EVENT::Manager::Register();
				SB_EVENT::EquipEventSink::Register();
				SB_EVENT::AttackAnimationEventSink::Register();
				SBO::EVENT::MASTERY::Register();
				UI::PRISMA::Bridge::GetSingleton()->Initialize();
				SB_OVERHAUL::Manager::GetSingleton()->PushUISnapshot();
				SB_OVERHAUL::Manager::GetSingleton()->PushHUDSnapshot();
				// Log plugin loaded
				LOG_INFO("{} loaded", DLLMAIN::Plugin::GetSingleton()->Info().name);
				break;
			}
		}
	}


	SKSE_API bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
	{
		if (a_skse->RuntimeVersion().minor() == 4) {
            return false;
        }
        // Load Plugin
		auto loadPlugin = DLLMAIN::Plugin::GetSingleton()->Load(a_skse);
		if (loadPlugin) {
			// Register SKSE::MessagingInterface
			if (!SKSE::GetMessagingInterface()->RegisterListener(SKSEPlugin_Message)) { return false; }
			SBIND::SERIALIZATION::Register();
		}
#if defined(DUMP_OFFSETS)
		DumpOffsets(a_skse->RuntimeVersion());
#endif
		return loadPlugin;
	}
}
