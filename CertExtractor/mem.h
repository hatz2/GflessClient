#ifndef MEM_H
#define MEM_H

#include <Windows.h>
#include <Psapi.h>
#include <QString>

#define READ_PTR(ptr, offset) *(uintptr_t*)(ptr + offset); if (ptr == 0) return 0;

DWORD PatternScan(const char* pattern, const char* mask, int offset = 0, const char* moduleName = nullptr);

void Nop(const DWORD dest, const unsigned int size);

void Patch(const DWORD dest, const char* src, const unsigned int size);

bool Hook(void* toHook, void* ourFunc, const int len);

MODULEINFO GetModuleInfo(LPCSTR name = NULL);

#endif // MEM_H
