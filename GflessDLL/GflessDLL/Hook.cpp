#include "Hook.h"

bool Hook(void* toHook, void* ourFunc, const int len)
{
    if (len < 5) return false;

    DWORD curProtection;
    VirtualProtect(toHook, len, PAGE_EXECUTE_READWRITE, &curProtection);

    memset(toHook, 0x90, len);

    DWORD relativeAdress = ((DWORD)ourFunc - (DWORD)toHook) - 5;

    *(BYTE*)toHook = 0xE9;
    *(DWORD*)((DWORD)toHook + 1) = relativeAdress;

    DWORD temp;
    VirtualProtect(toHook, len, curProtection, &temp);

    return true;
}