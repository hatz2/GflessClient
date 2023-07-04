#include "NosmallDisabler.h"
#include "Hook.h"
#include "PatternScanner.h"

#include <stdio.h>

namespace {
    DWORD nosmallAtStartAddress;
    constexpr int HOOK_SIZE = 5;
}

__declspec(naked) void disableNosmall() {
    __asm {
        ret
    }
}

void NosmallDisabler::Initialize()
{
    nosmallAtStartAddress = PatternScan(
        "\x55\x8b\xec\x6a\x00\x53\x8b\xda\x33\xc0\x55\x68\x00\x00\x00\x00\x64\xff\x00\x64\x89\x00\x8b\xc3\x8b\x10\xff\x52\x00\x48\x7c\x00\x8d\x4d\x00\x33\xd2\x8b\xc3\x8b\x18\xff\x53\x00\x8b\x45\x00\x33\xd2\xe8\x00\x00\x00\x00\x48", 
        "xxxx?xxxxxxx????xx?xx?xxxxxx?xx?xx?xxxxxxxx?xx?xxx????x",
        0
    );

    Hook(reinterpret_cast<void*>(nosmallAtStartAddress), reinterpret_cast<void*>(disableNosmall), HOOK_SIZE);
}