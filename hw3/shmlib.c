#define _POSIX_C_SOURCE 200809L
#include "shmlib.h"
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)

static int open_shared_mem(shm_t* s) {
    s->hMap = CreateFileMappingA(
        INVALID_HANDLE_VALUE, 
        NULL,
        PAGE_READWRITE,
        0,
        (DWORD)sizeof(shared_data_t),
        "Local\\Hw3_Shm"
    );
    if (!s->hMap) return 0;

    s->data = (shared_data_t*)MapViewOfFile(
        s->hMap,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(shared_data_t)
    );
    if (!s->data) {
        CloseHandle(s->hMap);
        s->hMap = NULL;
        return 0;
    }

    return 1;
}

static void close_shared_mem(shm_t* s) {
    if (s->data) {
        UnmapViewOfFile(s->data);
        s->data = NULL;
    }
    if (s->hMap) {
        CloseHandle(s->hMap);
        s->hMap = NULL;
    }
}

static int open_mutex(shm_t* s) {
    s->hMutex = CreateMutexA(
        NULL, 
        FALSE, 
        "Local\\Hw3_Mtx"
    );
    return s->hMutex != NULL ? 1 : 0;
}

static void close_mutex(shm_t* s) {
    if (s->hMutex) {
        CloseHandle(s->hMutex);
        s->hMutex = NULL;
    }
}

void shm_lock(shm_t* s) {
    WaitForSingleObject(s->hMutex, INFINITE);
}

void shm_unlock(shm_t* s) {
    ReleaseMutex(s->hMutex);
}

static void try_become_leader(shm_t* s) {
    s->hLeader = CreateMutexA(NULL, FALSE, "Local\\Hw3_Leader");
    if (!s->hLeader) return;

    if (WaitForSingleObject(s->hLeader, 0) == WAIT_OBJECT_0) {
        s->is_leader = 1;
        shm_lock(s);
        s->data->leader_pid = proc_self_pid();
        shm_unlock(s);
    }
}

static void release_leader(shm_t* s) {
    if (s->is_leader && s->hLeader) {
        ReleaseMutex(s->hLeader);
    }
    if (s->hLeader) {
        CloseHandle(s->hLeader);
        s->hLeader = NULL;
    }
    s->is_leader = 0;
}

#else

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

static int open_shared_mem(shm_t* s) {
    s->shm_fd = shm_open(
        "/hw3_shm", 
        O_CREAT | O_RDWR, 
        0666
    );
    if (s->shm_fd < 0) return 0;

    if (ftruncate(s->shm_fd, (off_t)sizeof(shared_data_t)) < 0) {
        close(s->shm_fd);
        s->shm_fd = -1;
        return 0;
    }

    s->data = (shared_data_t*)mmap(
        NULL,
        sizeof(shared_data_t),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        s->shm_fd,
        0
    );
    if (s->data == MAP_FAILED) {
        s->data = NULL;
        close(s->shm_fd);
        s->shm_fd = -1;
        return 0;
    }

    return 1;
}

static void close_shared_mem(shm_t* s) {
    if (s->data) {
        munmap(s->data, sizeof(shared_data_t));
        s->data = NULL;
    }
    if (s->shm_fd >= 0) {
        close(s->shm_fd);
        s->shm_fd = -1;
    }
}

static int open_mutex(shm_t* s) {
    s->mtx_sem = sem_open(
        "/hw3_mtx_sem", 
        O_CREAT, 
        0666, 
        1
    );
    return s->mtx_sem != SEM_FAILED ? 1 : 0;
}

static void close_mutex(shm_t* s) {
    if (s->mtx_sem && s->mtx_sem != SEM_FAILED) {
        sem_close(s->mtx_sem);
        s->mtx_sem = NULL;
    }
}

void shm_lock(shm_t* s) {
    sem_wait(s->mtx_sem);
}

void shm_unlock(shm_t* s) {
    sem_post(s->mtx_sem);
}

static void try_become_leader(shm_t* s) {
    s->leader_fd = open("/tmp/hw3_leader.lock", O_CREAT | O_RDWR, 0666);
    if (s->leader_fd < 0) return;

    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(s->leader_fd, F_SETLK, &fl) == 0) {
        s->is_leader = 1;
        shm_lock(s);
        s->data->leader_pid = proc_self_pid();
        shm_unlock(s);
    }
}

static void release_leader(shm_t* s) {
    if (s->is_leader && s->leader_fd >= 0) {
        struct flock fl;
        memset(&fl, 0, sizeof(fl));
        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;
        fcntl(s->leader_fd, F_SETLK, &fl);
    }
    if (s->leader_fd >= 0) {
        close(s->leader_fd);
        s->leader_fd = -1;
    }
    s->is_leader = 0;
}

#endif

int shm_init(shm_t* s) {
    if (!s) return 0;
    memset(s, 0, sizeof(*s));
#if !defined(_WIN32) && !defined(_WIN64)
    s->shm_fd = -1;
    s->leader_fd = -1;
    s->mtx_sem = SEM_FAILED;
#endif

    if (!open_mutex(s)) {
        shm_close(s);
        return 0;
    }

    if (!open_shared_mem(s)) {
        shm_close(s);
        return 0;
    }

    try_become_leader(s);
    return 1;
}

void shm_close(shm_t* s) {
    if (!s) return;
    release_leader(s);
    close_shared_mem(s);
    close_mutex(s);
}