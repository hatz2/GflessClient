#include "TClientCore.h"
#include "../PatternScanner.h"

TClientCore* TClientCore::getInstance()
{
    static DWORD patternAddress = PatternScan(
        "\xa1\x00\x00\x00\x00\x8b\x00\xe8\x00\x00\x00\x00\xe8\x00\x00\x00\x00\xa3",
        "x????xxx????x????x",
        1);

    DWORD address = patternAddress;
    address = READ_PTR(address, 0);
    address = READ_PTR(address, 0);
    address = READ_PTR(address, 0);
    address = READ_PTR(address, 4);

    TClientCore* instance = reinterpret_cast<TClientCore*>(address);

    if (IsBadReadPtr(instance, sizeof(TClientCore)))
        return nullptr;

    return instance;
}

uint32_t TClientCore::getSocket() const
{
    return socket;
}
