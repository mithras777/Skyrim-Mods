#pragma once

#include <Windows.h>

#include <filesystem>
#include <string>
#include <utility>

namespace SKSEMenuFramework
{
	using RenderFunction = void(__stdcall*)();

	namespace detail
	{
		inline HMODULE GetModule() noexcept
		{
			static HMODULE module = nullptr;
			if (!module) {
				module = GetModuleHandleA("SKSEMenuFramework.dll");
				if (!module) {
					module = GetModuleHandleA("SKSEMenuFramework");
				}
				if (!module) {
					module = LoadLibraryA("Data\\SKSE\\Plugins\\SKSEMenuFramework.dll");
				}
			}
			return module;
		}

		inline std::string& Section() noexcept
		{
			static std::string section;
			return section;
		}
	}

	inline bool IsInstalled()
	{
		return std::filesystem::exists("Data\\SKSE\\Plugins\\SKSEMenuFramework.dll");
	}

	inline void SetSection(std::string a_section)
	{
		detail::Section() = std::move(a_section);
	}

	inline void AddSectionItem(std::string a_name, RenderFunction a_renderer)
	{
		using AddSectionItemFunction = void (*)(const char*, RenderFunction);
		const auto module = detail::GetModule();
		const auto addSectionItem = module ? reinterpret_cast<AddSectionItemFunction>(GetProcAddress(module, "AddSectionItem")) : nullptr;
		if (addSectionItem) {
			addSectionItem((detail::Section() + "/" + a_name).c_str(), a_renderer);
		}
	}
}
