#ifndef INJECTOR_H
#define INJECTOR_H

#include <Windows.h>
#include <QDebug>

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
    {
        qDebug() << "Error on VirtualAllocEx:" << GetLastError();
        CloseHandle(hProc);
        return false;
    }

    if (!WriteProcessMemory(hProc, lpAllocMem, dllPath, strlen(dllPath) + 1, nullptr))
    {
        qDebug() << "Error on WriteProcessMemory:" << GetLastError();
        CloseHandle(hProc);
        return false;
    }

    hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, lpAllocMem, 0, NULL);

    if (!hThread)
    {
        qDebug() << "Error on CreateRemoteThread:" << GetLastError();

        if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
        {
            qDebug() << "Error on VirtualFreeEx:" << GetLastError();
        }

        CloseHandle(hProc);
        return false;
    }

    if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
    {
        qDebug() << "Error on WaitForSingleObject:" << GetLastError();

        if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
        {
            qDebug() << "Error on VirtualFreeEx:" << GetLastError();
        }

        CloseHandle(hThread);
        CloseHandle(hProc);
        return false;
    }

    if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
    {
        qDebug() << "Error on VirtualFreeEx:" << GetLastError();
    }

    CloseHandle(hThread);
    CloseHandle(hProc);
    return true;
}

#endif // INJECTOR_H
