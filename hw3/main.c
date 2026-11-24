#define _POSIX_C_SOURCE 200809L
#include "proclib.h"
#include "loglib.h"
#include "shmlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

static void ms_sleep(int ms) {
    Sleep((DWORD)ms);
}

static DWORD WINAPI input_thread(LPVOID arg) {
    shm_t* shm = (shm_t*)arg;
    char buf[128];
    while (fgets(buf, sizeof(buf), stdin)) {
        char* end = NULL;
        long long val = strtoll(buf, &end, 10);
        if (end != buf) {
            shm_lock(shm);
            shm->data->counter = val;
            shm_unlock(shm);
            printf("counter = %lld\n", val);
            fflush(stdout);
        }
    }
    return 0;
}

static void start_input_reader(shm_t* shm) {
    HANDLE h = CreateThread(NULL, 0, input_thread, shm, 0, NULL);
    if (h) CloseHandle(h);
}

static int is_ok_proc(proc_t p) {
    return p != NULL;
}

#else
#include <unistd.h>
#include <pthread.h>

static void ms_sleep(int ms) {
    usleep((useconds_t)ms * 1000u);
}

static void* input_thread(void* arg) {
    shm_t* shm = (shm_t*)arg;
    char buf[128];
    while (fgets(buf, sizeof(buf), stdin)) {
        char* end = NULL;
        long long val = strtoll(buf, &end, 10);
        if (end != buf) {
            shm_lock(shm);
            shm->data->counter = val;
            shm_unlock(shm);
            printf("counter = %lld\n", val);
            fflush(stdout);
        }
    }
    return NULL;
}

static void start_input_reader(shm_t* shm) {
    pthread_t th;
    if (pthread_create(&th, NULL, input_thread, shm) == 0) {
        pthread_detach(th);
    }
}

static int is_ok_proc(proc_t p) {
    return p > 0;
}
#endif

static const char* LOG_PATH = "audit.log";

static int do_copy1(shm_t* shm) {
    proc_id_t pid = proc_self_pid();
    char s[128];

    log_write(LOG_PATH, pid, "copy1 start");

    shm_lock(shm);
    shm->data->counter += 10;
    long long c = shm->data->counter;
    shm_unlock(shm);

    snprintf(s, sizeof(s), "copy1 +10 -> counter=%lld", c);
    log_write(LOG_PATH, pid, s);
    log_write(LOG_PATH, pid, "copy1 exit");
    return 0;
}

static int do_copy2(shm_t* shm) {
    proc_id_t pid = proc_self_pid();
    char s[128];

    log_write(LOG_PATH, pid, "copy2 start");

    shm_lock(shm);
    shm->data->counter *= 2;
    long long c1 = shm->data->counter;
    shm_unlock(shm);

    snprintf(s, sizeof(s), "copy2 *2 -> counter=%lld", c1);
    log_write(LOG_PATH, pid, s);

    ms_sleep(2000);

    shm_lock(shm);
    shm->data->counter /= 2;
    long long c2 = shm->data->counter;
    shm_unlock(shm);

    snprintf(s, sizeof(s), "copy2 /2 -> counter=%lld", c2);
    log_write(LOG_PATH, pid, s);
    log_write(LOG_PATH, pid, "copy2 exit");
    return 0;
}

static void spawn_copy(proc_t* p, int* running, const char* prog,
                       const char* flag, const char* name, proc_id_t pid) {
    char s[128];

    if (*running) {
        if (proc_is_running(*p)) {
            snprintf(s, sizeof(s), "%s still running, skip", name);
            log_write(LOG_PATH, pid, s);
            return;
        }
        proc_wait(*p);
        *running = 0;
    }

    proc_t child = proc_start(prog, flag, "");
    if (is_ok_proc(child)) {
        *p = child;
        *running = 1;
        snprintf(s, sizeof(s), "spawned %s", name);
        log_write(LOG_PATH, pid, s);
    }
}

int main(int argc, char** argv) {
    const char* prog = (argc > 0 && argv[0]) ? argv[0] : "app";
    const char* mode = (argc > 1 && argv[1]) ? argv[1] : "";

    shm_t shm;
    if (!shm_init(&shm)) {
        printf("shm_init error\n");
        return 1;
    }

    if (strcmp(mode, "--copy1") == 0) {
        int r = do_copy1(&shm);
        shm_close(&shm);
        return r;
    }

    if (strcmp(mode, "--copy2") == 0) {
        int r = do_copy2(&shm);
        shm_close(&shm);
        return r;
    }

    proc_id_t pid = proc_self_pid();
    char s[128];
    snprintf(s, sizeof(s), "start (leader=%d)", shm.is_leader);
    log_write(LOG_PATH, pid, s);
    printf("PID %llu started. Type number + Enter to set counter:\n", (unsigned long long)pid);
    fflush(stdout);

    start_input_reader(&shm);

    proc_t p1 = {0}, p2 = {0};
    int r1 = 0, r2 = 0;
    int t300 = 0, t1000 = 0, t3000 = 0;

    while (1) {
        ms_sleep(100);
        t300 += 100;
        t1000 += 100;
        t3000 += 100;

        if (t300 >= 300) {
            t300 = 0;
            shm_lock(&shm);
            shm.data->counter += 1;
            shm_unlock(&shm);
        }

        if (t1000 >= 1000) {
            t1000 = 0;
            if (shm.is_leader) {
                shm_lock(&shm);
                long long c = shm.data->counter;
                shm_unlock(&shm);
                snprintf(s, sizeof(s), "counter=%lld", c);
                log_write(LOG_PATH, pid, s);
            }
        }

        if (t3000 >= 3000) {
            t3000 = 0;
            if (shm.is_leader) {
                spawn_copy(&p1, &r1, prog, "--copy1", "copy1", pid);
                spawn_copy(&p2, &r2, prog, "--copy2", "copy2", pid);
            }
        }
    }

    shm_close(&shm);
    return 0;
}