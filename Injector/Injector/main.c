#include <stdlib.h>
#include <stdio.h>
#include "injector.h"

int main(int argc, char** argv) {
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (argc != 3) {
        printf("Wrong arguments\n");
        printf("Program must be started with the following arguments: PID DLL_PATH\n");
        system("pause");
        return EXIT_FAILURE;
    }

    DWORD pid = strtol(argv[1], NULL, 10);
    char* dllPath = argv[2];
    int injected = 0;

    if (pid == 0L) {
        printf("No valid conversion from strtol\n");
        system("pause");
        return EXIT_FAILURE;
    }

    if (pid == LONG_MAX || pid == LONG_MIN) {
        printf("strtol value is out of range. Errno: %x\n", errno);
        system("pause");
        return EXIT_FAILURE;
    }

    injected = Inject(pid, dllPath);

    if (!injected) {
        printf("%s injected successfully on process %u\n", dllPath, pid);
    }

    return injected;
}