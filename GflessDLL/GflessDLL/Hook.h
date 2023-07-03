#pragma once

#include <Windows.h>

bool Hook(void* toHook, void* ourFunc, const int len);
