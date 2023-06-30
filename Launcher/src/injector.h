#ifndef INJECTOR_H
#define INJECTOR_H

#include <Windows.h>
#include <QDebug>

bool Inject(DWORD pid, const char* dllPath)
{
    HANDLE hProc;
    HANDLE hThread;
    LPVOID lpAllocMem;
    DWORD exitCode;

    // Retrieve a handle to the game process
    hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (!hProc)
        return false;

    // Allocate memory in the target process to write the DLL path
    lpAllocMem =  VirtualAllocEx(hProc, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    if (!lpAllocMem)
    {
        qDebug() << "Error on VirtualAllocEx:" << GetLastError();
        CloseHandle(hProc);
        return false;
    }

    // Write the DLL path into the allocated memory
    if (!WriteProcessMemory(hProc, lpAllocMem, dllPath, strlen(dllPath) + 1, nullptr))
    {
        qDebug() << "Error on WriteProcessMemory:" << GetLastError();
        CloseHandle(hProc);
        return false;
    }

    // Create a thread in the target memory that calls LoadLibraryA with the path to the DLL
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

    // Waits for thread execution to finish
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

    // Retrieve the exit code of the thread
    if (!GetExitCodeThread(hThread, &exitCode))
    {
        qDebug() << "Error on GetExitCodeThread:" << GetLastError();

        if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
        {
            qDebug() << "Error on VirtualFreeEx:" << GetLastError();
        }

        CloseHandle(hThread);
        CloseHandle(hProc);
        return false;
    }

    // Check if there was any error during the execution of LoadLibraryA in the target process
    if (!exitCode)
    {
        qDebug() << "Error in the execution of LoadLibraryA in the target process:" << exitCode;

        if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
        {
            qDebug() << "Error on VirtualFreeEx:" << GetLastError();
        }

        CloseHandle(hThread);
        CloseHandle(hProc);
        return false;
    }

    // Frees the allocated memory and close the process and thread handles
    if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
    {
        qDebug() << "Error on VirtualFreeEx:" << GetLastError();
    }

    CloseHandle(hThread);
    CloseHandle(hProc);
    return true;
}

#endif // INJECTOR_H
