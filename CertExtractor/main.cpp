#include <QCoreApplication>
#include <QDebug>
#include "mem.h"
#include <iostream>
#include <Windows.h>
#include <QFile>

#pragma comment(lib, "User32.lib")

constexpr int certFileHookSize = 6;
QFile* certFile = nullptr;
DWORD certFileHookJumpBackAddr = 0;

constexpr int saveCertHookSize = 6;
DWORD saveCertJumpBackAddr = 0;

constexpr int importPkcs12HookSize = 5;
QByteArray* passPhrase = nullptr;
DWORD importPkcs12JumpBackAddr = 0;
DWORD test = 0;


void __stdcall saveCert() {
    QByteArray content = certFile->readAll();
    QString savePath = "C:/Users/root/Documents/client.p12";
    QFile outFile(savePath);

    if (outFile.open(QFile::WriteOnly | QFile::Truncate)) {
        outFile.write(content);
        std::cout << "Certificate saved at " << savePath.toStdString() << std::endl;
        outFile.close();
    }
    else {
        std::cout << "Could not open file " << savePath.toStdString() << std::endl;
    }
}


void __declspec(naked) certFileHookFun() {
    __asm {
        mov certFile, ecx
        mov dword ptr ds:[eax], 1
        jmp certFileHookJumpBackAddr
    }
}


void __declspec(naked) saveCertHookFun() {
    __asm {
        pushad
        pushfd
        call saveCert
        popfd
        popad
        lea ecx,[ebp-0x42c]
        jmp saveCertJumpBackAddr
    }
}


void __stdcall printPassPhrase() {
    std::cout << "Passphrase: " << passPhrase->toStdString() << std::endl;
}


void __declspec(naked) importPkcs12HookFun() {
    __asm {
        push eax
        mov eax, [esp + 0x40] // 0xC3 + 0x04 because of the push eax
        mov passPhrase, eax
        pop eax
        pushad
        pushfd
        call printPassPhrase
        popfd
        popad
        lea eax, [esp + 0x08]
        push ebx
        jmp importPkcs12JumpBackAddr
    }
}


int MainFunc()
{
    int argc = 0;
    QCoreApplication a(argc, nullptr);

    FILE* file;
    AllocConsole();
    freopen_s(&file, "CONOUT$", "w", stdout);

    DWORD certFileHookAddr = PatternScan(
        "\xc7\x00\x00\x00\x00\x00\xff\x15\x00\x00\x00\x00\x84\xc0\x75\x00\x8d\x8d\x00\x00\x00\x00\xff\x15",
        "xx????xx????xxx?xx????xx"
    );
    DWORD saveCertHookAddr = PatternScan(
        "\x8d\x8d\x00\x00\x00\x00\xff\x15\x00\x00\x00\x00\x8d\x8d\x00\x00\x00\x00\xff\x15\x00\x00\x00\x00\x6a",
        "xx????xx????xx????xx????x"
    );

    DWORD importPkcs12HookAddr = PatternScan(
        "\x8d\x44\x24\x00\x53\x56\x57\x50",
        "xxx?xxxx",
        0,
        "Qt5Network.dll"
    );

    std::cout << "Cert file hook address: " << std::hex  << certFileHookAddr << std::endl;
    std::cout << "Save cert hook address: " << std::hex <<  saveCertHookAddr << std::endl;
    std::cout << "ImportPkcs12 hook address: " << std::hex <<  importPkcs12HookAddr << std::endl << std::endl;

    certFileHookJumpBackAddr = certFileHookAddr + certFileHookSize;
    saveCertJumpBackAddr = saveCertHookAddr + saveCertHookSize;
    importPkcs12JumpBackAddr = importPkcs12HookAddr + importPkcs12HookSize;

    if (certFileHookAddr && saveCertHookAddr && importPkcs12HookAddr) {
        Hook((void*)certFileHookAddr, (void*)certFileHookFun, certFileHookSize);
        Hook((void*)saveCertHookAddr, (void*)saveCertHookFun, saveCertHookSize);
        Hook((void*)importPkcs12HookAddr, (void*)importPkcs12HookFun, importPkcs12HookSize);
    }

    return a.exec();
}


DWORD WINAPI GuiStart(LPVOID param)
{
    Q_UNUSED(param);

    FreeLibraryAndExitThread((HMODULE)param, MainFunc());
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    Q_UNUSED(lpReserved);

    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        CloseHandle(CreateThread(NULL, 0, GuiStart, hModule, 0, NULL));
    }

    return TRUE;
}
