#include "stdafx.h"
#define DLL_EXPORT extern "C" __declspec(dllexport)
void DllInit(HMODULE);

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved)
{
    HANDLE hThread = INVALID_HANDLE_VALUE;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)DllInit, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
