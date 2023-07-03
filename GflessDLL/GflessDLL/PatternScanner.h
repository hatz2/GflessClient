#pragma once
#include <Windows.h>
#include <Psapi.h>

#define READ_PTR(ptr, offset) *(uintptr_t*)(ptr + offset); if (ptr == 0) return 0;

DWORD PatternScan(const char* pattern, const char* mask, int offset);
