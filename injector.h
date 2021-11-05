#ifndef INJECTOR_H
#define INJECTOR_H

#include <Windows.h>

bool Inject(DWORD pid, const char* dllPath)
{
    HANDLE hProc;
    HANDLE hThread;
    LPVOID lpAllocMem;

    hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (!hProc)
        return false;

    lpAllocMem =  VirtualAllocEx(hProc, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    if (!lpAllocMem)
        return false;

    WriteProcessMemory(hProc, lpAllocMem, dllPath, strlen(dllPath) + 1, nullptr);
    hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, lpAllocMem, 0, NULL);

    if (!hThread)
        return false;

    CloseHandle(hThread);
    CloseHandle(hProc);

    return true;
}

#endif // INJECTOR_H
