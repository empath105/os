#ifndef PROCLIB_H
#define PROCLIB_H

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

typedef HANDLE proc_t;
typedef DWORD proc_id_t;

#else
#include <sys/types.h>
typedef pid_t proc_t;
typedef pid_t proc_id_t;
#endif

proc_t proc_start(const char* name, const char* arg1, const char* arg2);
int proc_wait(proc_t proc);
int proc_is_running(proc_t proc);
proc_id_t proc_self_pid(void);

#endif