#include "GameStructures/TNTNewServerSelectWidget2.h"
#include "GameStructures/TCharacterSelectWidget.h"
#include "PatternScanner.h"
#include "NosmallDisabler.h"
#include <Windows.h>
#include <string>
#include <iostream>

int main()
{
#ifdef _DEBUG
    FILE* file;
    AllocConsole();
    freopen_s(&file, "CONOUT$", "w", stdout);
#endif // _DEBUG

    const int BUFFER_SIZE = 255;
    const char* pipeName = "\\\\.\\pipe\\GflessClient";
    int language, server, channel, character;
    bool autoLogin, disableNosmall;
    HANDLE hPipe;
    DWORD dwMode;
    BOOL fSuccess;
    std::string message;
    char readBuffer[BUFFER_SIZE];
    TNTNewServerSelectWidget2* newServerSelectWidget = nullptr;
    TCharacterSelectWidget* characterSelectWidget = nullptr;
    DWORD pid = GetCurrentProcessId();

    // Set up a pipe to communicate with the Gfless Client
    // and receive the values for the language server,
    // the server, the channel and the character slot
    // Also used to get proxy configuration
    hPipe = CreateFileA(pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hPipe == INVALID_HANDLE_VALUE)
        if (!WaitNamedPipeA(pipeName, 20000))
            return EXIT_FAILURE;

    dwMode = PIPE_READMODE_BYTE | PIPE_WAIT;
    fSuccess = SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    // Send the disable nosmall message
    message = std::to_string(pid) + " DisableNosmall";
    fSuccess = WriteFile(hPipe, message.c_str(), message.size(), NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    ZeroMemory(readBuffer, BUFFER_SIZE);
    fSuccess = ReadFile(hPipe, readBuffer, BUFFER_SIZE, NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    try {
        disableNosmall = std::stoi(std::string(readBuffer));
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    if (disableNosmall) {
        NosmallDisabler::Initialize();
    }
    
    // Send the autologin message
    message = std::to_string(pid) + " AutoLogin";
    fSuccess = WriteFile(hPipe, message.c_str(), message.size(), NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    ZeroMemory(readBuffer, BUFFER_SIZE);
    fSuccess = ReadFile(hPipe, readBuffer, BUFFER_SIZE, NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    try {
        autoLogin = std::stoi(std::string(readBuffer));
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Send the server language message
    message = std::to_string(pid) + " ServerLanguage";
    fSuccess = WriteFile(hPipe, message.c_str(), message.size(), NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    ZeroMemory(readBuffer, BUFFER_SIZE);
    fSuccess = ReadFile(hPipe, readBuffer, BUFFER_SIZE, NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    language = readBuffer[0] - 0x30;

    // Send the server message
    message = std::to_string(pid) + " Server";
    fSuccess = WriteFile(hPipe, message.c_str(), message.size(), NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    ZeroMemory(readBuffer, BUFFER_SIZE);
    fSuccess = ReadFile(hPipe, readBuffer, BUFFER_SIZE, NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    server = readBuffer[0] - 0x30;

    // Send the channel message
    message = std::to_string(pid) + " Channel";
    fSuccess = WriteFile(hPipe, message.c_str(), message.size(), NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    ZeroMemory(readBuffer, BUFFER_SIZE);
    fSuccess = ReadFile(hPipe, readBuffer, BUFFER_SIZE, NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    channel = readBuffer[0] - 0x30;

    // Send the character message
    message = std::to_string(pid) + " Character";
    fSuccess = WriteFile(hPipe, message.c_str(), message.size(), NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    ZeroMemory(readBuffer, BUFFER_SIZE);
    fSuccess = ReadFile(hPipe, readBuffer, BUFFER_SIZE, NULL, NULL);

    if (!fSuccess)
        return EXIT_FAILURE;

    character = readBuffer[0] - 0x30;
    character = character - 1;

    // Initialize widget structures
    while (newServerSelectWidget == nullptr || characterSelectWidget == nullptr)
    {
        newServerSelectWidget = TNTNewServerSelectWidget2::getInstance();
        characterSelectWidget = TCharacterSelectWidget::getInstance();
        Sleep(500);
    }

    if (autoLogin) {
        // Wait for the login widget to be visible
        // and log into the desired server and channel
        while (!characterSelectWidget->isVisible()) {
            while (!newServerSelectWidget->isVisible())
                Sleep(500);

            newServerSelectWidget->selectLanguage(language);
            Sleep(2000);

            while (!newServerSelectWidget->isVisible())
                Sleep(500);

            newServerSelectWidget->selectServer(server);
            Sleep(1000);
            newServerSelectWidget->selectChannel(channel);
            Sleep(4000);
        }

        Sleep(500);

        if (character >= 0)
        {
            characterSelectWidget->clickCharacterButton(character);
            Sleep(1000);
            characterSelectWidget->clickStartButton();
        }
    }

#ifdef _DEBUG
    fclose(file);
    FreeConsole();
#endif // _DEBUG


    return EXIT_SUCCESS;
}

DWORD WINAPI DllStart(LPVOID param)
{
    FreeLibraryAndExitThread((HMODULE)param, main());
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    HANDLE hThread;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        hThread = CreateThread(NULL, NULL, DllStart, hModule, NULL, NULL);
        if (hThread != NULL) CloseHandle(hThread);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
