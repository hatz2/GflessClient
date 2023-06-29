#include "PatternScanner.h"

MODULEINFO GetModuleInfo()
{
    MODULEINFO modinfo = { 0 };
    HMODULE hModule = GetModuleHandle(NULL);

    if (hModule == 0)
        return modinfo;

    GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
    return modinfo;
}

DWORD PatternScan(const char* pattern, const char* mask, int offset)
{
    MODULEINFO mInfo = GetModuleInfo();

    DWORD base = (DWORD)mInfo.lpBaseOfDll;
    DWORD size = (DWORD)mInfo.SizeOfImage;

    auto patternLength = strlen(mask);

    for (unsigned i = 0; i < size - patternLength; i++)
    {
        bool found = true;
        for (unsigned j = 0; j < patternLength; j++)
            found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);

        if (found)
            return base + i + offset;
    }

    return NULL;
}