#ifndef PROCESSCHECKER_H
#define PROCESSCHECKER_H

#include <Windows.h>
#include <TlHelp32.h>

bool isProcessRunning(const wchar_t* processName)
{
    bool exists = false;
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (hSnapshot == INVALID_HANDLE_VALUE)
        return false;

    if (Process32FirstW(hSnapshot, &entry))
    {
        do
        {
            if (!_wcsicmp(entry.szExeFile, processName))
            {
                exists = true;
                break;
            }

        } while (Process32NextW(hSnapshot, &entry));
    }

    CloseHandle(hSnapshot);

    return exists;
}

void killProcess(const wchar_t* processName)
{
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (hSnapshot == INVALID_HANDLE_VALUE)
        return;

    if (Process32FirstW(hSnapshot, &entry))
    {
        do
        {
            if (!_wcsicmp(entry.szExeFile, processName))
            {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);

                if (hProcess != NULL)
                {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }

                break;
            }

        } while (Process32NextW(hSnapshot, &entry));
    }

    CloseHandle(hSnapshot);
}

#endif // PROCESSCHECKER_H
