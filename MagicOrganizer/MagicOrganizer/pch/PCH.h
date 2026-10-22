#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <REX/REX/Singleton.h>

#include "util/HookUtil.h"

using namespace std::literals;

#define RELOCATION_OFFSET(a_se, a_ae) REL::Relocate((a_se), (a_ae))
