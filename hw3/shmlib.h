#ifndef SHMLIB_H
#define SHMLIB_H

#include "proclib.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <semaphore.h>
#endif

typedef struct {
    long long counter;
    proc_id_t leader_pid;
    int stop_flag;
} shared_data_t;

typedef struct {
    shared_data_t* data;
    int is_leader;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE hMap;
    HANDLE hMutex;
    HANDLE hLeader;
#else
    int shm_fd;
    int leader_fd;
    sem_t* mtx_sem;
#endif
} shm_t;

int shm_init(shm_t* s);
void shm_lock(shm_t* s);
void shm_unlock(shm_t* s);
void shm_close(shm_t* s);

#endif