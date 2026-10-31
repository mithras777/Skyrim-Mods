#include "api/skse_api.h"
#include "plugin.h"
#include "version.h"
#include "util/LogUtil.h"

SKSEPluginInfo(
    .Version = REL::Version{ VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD },
    .Name = VERSION_PRODUCTNAME_STR,
    .Author = VERSION_AUTHOR_STR,
    .StructCompatibility = SKSE::StructCompatibility::Dependent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary
);

namespace SKSE {
    static void OnMessage(MessagingInterface::Message* message)
    {
        switch (message->type) {
        case MessagingInterface::kPostPostLoad:
            ::stl::Hook::install();
            break;
        case MessagingInterface::kDataLoaded:
            
            LOG_INFO("{} loaded", DLLMAIN::Plugin::GetSingleton()->Info().name);
            break;
        default:
            break;
        }
    }

    SKSE_API bool SKSEPlugin_Load(const SKSE::LoadInterface* skse)
    {
        const auto runtime = skse->RuntimeVersion();
        if (runtime.minor() == 4) {
            return false; // The starter is built for SE/AE, with VR disabled.
        }
        if (!DLLMAIN::Plugin::GetSingleton()->Load(skse)) {
            return false;
        }
        return SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    }
}
