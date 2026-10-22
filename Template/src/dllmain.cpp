#include <Windows.h>

BOOL APIENTRY DllMain(HMODULE a_module, DWORD a_reason, LPVOID)
{
	if (a_reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(a_module);
	}
	return TRUE;
}
