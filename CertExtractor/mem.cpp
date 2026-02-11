#include "mem.h"

MODULEINFO GetModuleInfo(LPCSTR name)
{
    MODULEINFO modinfo = { 0 };
    HMODULE hModule = GetModuleHandleA(name);

    if (hModule == 0)
        return modinfo;

    GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
    return modinfo;
}

DWORD PatternScan(const char *pattern, const char *mask, int offset, const char *moduleName)
{
    MODULEINFO mInfo = GetModuleInfo(moduleName);

    DWORD base = (DWORD)mInfo.lpBaseOfDll;
    DWORD size = (DWORD)mInfo.SizeOfImage;

    DWORD patternLength = (DWORD)strlen(mask);

    for (DWORD i = 0; i < size - patternLength; i++)
    {
        bool found = true;
        for (DWORD j = 0; j < patternLength; j++)
        {
            if ((char*)(base + i + j) != nullptr)
                found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
        }

        if (found)
            return base + i + offset;
    }

    return NULL;
}

void Nop(const DWORD dest, const unsigned int size)
{
    if (!dest) return;

    DWORD oldProtect;
    VirtualProtect((LPVOID)dest, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset((LPVOID)dest, 0x90, size);
    VirtualProtect((LPVOID)dest, size, oldProtect, &oldProtect);
}

void Patch(const DWORD dest, const char *src, const unsigned int size)
{
    if (!dest) return;

    DWORD oldprotect;
    VirtualProtect((LPVOID)dest, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy((LPVOID)dest, src, size);
    VirtualProtect((LPVOID)dest, size, oldprotect, &oldprotect);
}

bool Hook(void *toHook, void *ourFunc, const int len)
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
