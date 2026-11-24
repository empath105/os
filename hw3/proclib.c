#include "proclib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)

proc_t proc_start(const char* name, const char* arg1, const char* arg2) {
    char cmd_line[512];
    snprintf(cmd_line, sizeof(cmd_line), "\"%s\" \"%s\" \"%s\"", name, arg1, arg2);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return NULL;
    }

    CloseHandle(pi.hThread);
    return pi.hProcess;
}

int proc_wait(proc_t proc) {
    if (!proc) return -1;

    WaitForSingleObject(proc, INFINITE);

    DWORD code;
    GetExitCodeProcess(proc, &code);
    CloseHandle(proc);
    return (int)code;
}

int proc_is_running(proc_t proc) {
    if (!proc) return 0;
    DWORD res = WaitForSingleObject(proc, 0);
    return (res == WAIT_TIMEOUT) ? 1 : 0;
}

proc_id_t proc_self_pid(void) {
    return GetCurrentProcessId();
}


#else
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

proc_t proc_start(const char* name, const char* arg1, const char* arg2) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (!pid) {
        execlp(name, name, arg1, arg2, (char*)NULL);
        perror("exec");
        exit(-2);
    }
    return pid;
}

int proc_wait(proc_t proc) {
    int status;
    if (proc < 0) return -1;

    wait(&status);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return -3;
}

int proc_is_running(proc_t proc) {
    if (proc <= 0) return 0;
    int status;
    pid_t r = waitpid(proc, &status, WNOHANG);
    if (r == 0) {
        return 1;
    }
    return 0;
}

proc_id_t proc_self_pid(void) {
    return getpid();
}
#endif