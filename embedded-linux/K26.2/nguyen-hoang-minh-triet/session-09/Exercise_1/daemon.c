#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sensor_shm.h"

/* --- Constants --- */
#define SLEEP_INTERVAL_SEC  2
#define FILE_LOADAVG        "/proc/loadavg"
#define FILE_MEMINFO        "/proc/meminfo"
#define MAX_LINE_SIZE       128

/* --- Global State for Graceful Shutdown --- */
volatile sig_atomic_t g_is_running = 1;

static void handle_sigint(int signum) {
    (void)signum;
    g_is_running = 0;
}

/* --- Helpers for System Data --- */
static int get_simulated_temp(double *temp_out) {
    FILE *file = fopen(FILE_LOADAVG, "r");
    if (!file) return -1;

    double load1;
    if (fscanf(file, "%lf", &load1) == 1) {
        *temp_out = 40.0 + (load1 * 10.0);
        fclose(file);
        return 0;
    }
    fclose(file);
    return -1;
}

static int get_mem_used_pct(double *pct_out) {
    FILE *file = fopen(FILE_MEMINFO, "r");
    if (!file) return -1;

    long mem_total = -1;
    long mem_free = -1;
    char line[MAX_LINE_SIZE];

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%ld", &mem_total);
        } else if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line + 8, "%ld", &mem_free);
        }
    }
    fclose(file);

    if (mem_total > 0 && mem_free >= 0) {
        *pct_out = (double)(mem_total - mem_free) / mem_total * 100.0;
        return 0;
    }
    return -1;
}

/* --- Main Daemon --- */
int main(void) {
    /* 1. Register SIGINT handler */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    /* 2. Create Shared Memory Segment */
    int shm_id = shmget(SHM_KEY, sizeof(sensor_data_t), IPC_CREAT | SHM_PERMISSIONS);
    if (shm_id < 0) {
        perror("[Daemon] shmget failed");
        exit(EXIT_FAILURE);
    }

    /* 3. Attach to Shared Memory */
    sensor_data_t *shm_ptr = (sensor_data_t *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("[Daemon] shmat failed");
        /* Attempt to clean up before exiting */
        shmctl(shm_id, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    printf("[Daemon] Shared memory created. Key=0x%X\n", SHM_KEY);

    /* 4. Main Update Loop */
    while (g_is_running) {
        double temp = 0.0;
        double mem_pct = 0.0;

        /* Gather sensor data safely */
        if (get_simulated_temp(&temp) < 0) temp = 0.0;
        if (get_mem_used_pct(&mem_pct) < 0) mem_pct = 0.0;

        /* Write to shared memory (Assuming atomic enough for this simple exercise. 
         * In a real critical system, you might need POSIX semaphores here) */
        shm_ptr->timestamp    = time(NULL);
        shm_ptr->cpu_temp     = temp;
        shm_ptr->ram_used_pct = mem_pct;

        printf("[Daemon] Written: temp=%.2f ram=%.2f%%\n", temp, mem_pct);

        /* Sleep. If SIGINT occurs, sleep will be interrupted (EINTR) and loop will break */
        unsigned int time_left = SLEEP_INTERVAL_SEC;
        while (time_left > 0 && g_is_running) {
            time_left = sleep(time_left);
        }
    }

    /* 5. Cleanup on Exit */
    printf("\n[Daemon] Cleaning up shared memory. Goodbye.\n");
    
    if (shmdt(shm_ptr) < 0) {
        perror("[Daemon] shmdt failed");
    }
    
    if (shmctl(shm_id, IPC_RMID, NULL) < 0) {
        perror("[Daemon] shmctl(IPC_RMID) failed");
    }

    return EXIT_SUCCESS;
}
