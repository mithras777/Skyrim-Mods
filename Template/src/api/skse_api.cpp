#include "api/skse_api.h"

#include "util/LogUtil.h"
#include "version.h"

#include <spdlog/sinks/basic_file_sink.h>

namespace {
	void InitializeLogging()
	{
		const auto logDirectory = SKSE::log::log_directory();
		if (!logDirectory) {
			return;
		}

		const auto logPath = *logDirectory / (std::string(VERSION_PRODUCTNAME_STR) + ".log");
		auto logger = spdlog::basic_logger_mt("skse-plugin", logPath.string(), true);
		logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
		logger->set_level(spdlog::level::trace);
		logger->flush_on(spdlog::level::trace);
		spdlog::set_default_logger(std::move(logger));
	}

	void OnSKSEMessage(SKSE::MessagingInterface::Message* a_message)
	{
		if (a_message && a_message->type == SKSE::MessagingInterface::kDataLoaded) {
			LOG_INFO("{} loaded", VERSION_PRODUCTNAME_STR);
		}
	}
}

namespace SKSE {
	SKSE_API constinit auto SKSEPlugin_Version = []() noexcept {
		PluginVersionData pluginData{};
		pluginData.PluginName(VERSION_PRODUCTNAME_STR);
		pluginData.PluginVersion(REL::Version(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD));
		pluginData.AuthorName(VERSION_AUTHOR_STR);
		pluginData.UsesAddressLibrary();
		pluginData.UsesNoStructs();
		// Keep the legacy 1.5.97 runtime explicitly compatible; newer SE/AE
		// runtimes use the Address Library compatibility flag above.
		pluginData.CompatibleVersions({ REL::Version(1, 5, 97, 0) });
		return pluginData;
	}();

	SKSE_API bool SKSEPlugin_Query(const QueryInterface* a_skse, PluginInfo* a_pluginInfo)
	{
		if (!a_skse || !a_pluginInfo || a_skse->IsEditor()) {
			return false;
		}

		// This starter project targets Skyrim SE/AE and intentionally rejects VR.
		if (a_skse->RuntimeVersion().minor() == 4) {
			return false;
		}

		a_pluginInfo->infoVersion = PluginInfo::kVersion;
		a_pluginInfo->name = VERSION_PRODUCTNAME_STR;
		a_pluginInfo->version = REL::Version(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD).pack();
		return true;
	}

	SKSE_API bool SKSEPlugin_Load(const LoadInterface* a_skse)
	{
		if (!a_skse) {
			return false;
		}

		SKSE::Init(a_skse, false);
		InitializeLogging();
		LOG_INFO("{} v{} initializing", VERSION_PRODUCTNAME_STR, VERSION_STR);

		const auto messaging = SKSE::GetMessagingInterface();
		if (messaging) {
			messaging->RegisterListener(OnSKSEMessage);
		}
		return true;
	}
}
