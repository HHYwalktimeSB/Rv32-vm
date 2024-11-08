#include "RVVM32.h"
#include<Windows.h>

extern MyVm* pVM;
extern int MemoryIoGuard;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        pVM = nullptr;
        MemoryIoGuard = 0;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        //if (MemoryIoGuard==0&&pVM) {
            //delete pVM;
            //pVM = nullptr;
        //}
        break;
    }
    return TRUE;
}