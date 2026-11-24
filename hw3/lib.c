#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

int run_and_wait(const char* name, const char* arg1, const char* arg2) {
    char cmd_line[512];
    snprintf(cmd_line, sizeof(cmd_line), "\"%s\" \"%s\" \"%s\"", name, arg1, arg2);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD code;
    GetExitCodeProcess(pi.hProcess, &code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)code;
}

#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int run_and_wait(const char* name, const char* arg1, const char* arg2) {
    int pid, status;
    
    if ((pid = fork()) < 0) {
        perror("fork");
        return -1;
    } else if (!pid) {
        execlp(name, name, arg1, arg2, (char*)NULL);
        perror("exec");
        exit(-2);
    }
    
    wait(&status);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
        
    return -3;
}
#endif