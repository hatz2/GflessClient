#include "injector.h"
#include <stdio.h>

int Inject(DWORD pid, const char* dllPath)
{
    HANDLE hProc;
    HANDLE hThread;
    LPVOID lpAllocMem;
    DWORD exitCode;

    // Retrieve a handle to the game process
    hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (!hProc)
        return EXIT_FAILURE;

    // Allocate memory in the target process to write the DLL path
    lpAllocMem = VirtualAllocEx(hProc, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    if (!lpAllocMem)
    {
        printf("Error on VirtualAllocEx: %u\n", GetLastError());
        CloseHandle(hProc);
        return EXIT_FAILURE;
    }

    // Write the DLL path into the allocated memory
    if (!WriteProcessMemory(hProc, lpAllocMem, dllPath, strlen(dllPath) + 1, NULL))
    {
        printf("Error on WriteProcessMemory: %u\n", GetLastError());
        CloseHandle(hProc);
        return EXIT_FAILURE;
    }

    // Create a thread in the target memory that calls LoadLibraryA with the path to the DLL
    hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, lpAllocMem, 0, NULL);

    if (!hThread)
    {
        printf("Error on CreateRemoteThread: %u\n", GetLastError());

        if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
        {
            printf("Error on VirtualFreeEx: %u\n", GetLastError());
        }

        CloseHandle(hProc);
        return EXIT_FAILURE;
    }

    // Waits for thread execution to finish
    if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
    {
        printf("Error on WaitForSingleObject: %u\n", GetLastError());

        if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
        {
            printf("Error on VirtualFreeEx: %u\n", GetLastError());
        }

        CloseHandle(hThread);
        CloseHandle(hProc);
        return EXIT_FAILURE;
    }

    // Retrieve the exit code of the thread
    if (!GetExitCodeThread(hThread, &exitCode))
    {
        printf("Error on GetExitCodeThread: %u\n", GetLastError());

        if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
        {
            printf("Error on VirtualFreeEx: %u\n", GetLastError());
        }

        CloseHandle(hThread);
        CloseHandle(hProc);
        return EXIT_FAILURE;
    }

    // Check if there was any error during the execution of LoadLibraryA in the target process
    if (!exitCode)
    {
        printf("Error in the execution of LoadLibraryA in the target process: %u\n", exitCode);

        if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
        {
            printf("Error on VirtualFreeEx: %u\n", GetLastError());
        }

        CloseHandle(hThread);
        CloseHandle(hProc);
        return EXIT_FAILURE;
    }

    // Frees the allocated memory and close the process and thread handles
    if (!VirtualFreeEx(hProc, lpAllocMem, 0, MEM_RELEASE))
    {
        printf("Error on VirtualFreeEx: %u\n", GetLastError());
    }

    CloseHandle(hThread);
    CloseHandle(hProc);
    return EXIT_SUCCESS;
}
