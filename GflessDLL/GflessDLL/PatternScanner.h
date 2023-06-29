#pragma once
#include <Windows.h>
#include <Psapi.h>

DWORD PatternScan(const char* pattern, const char* mask, int offset);
