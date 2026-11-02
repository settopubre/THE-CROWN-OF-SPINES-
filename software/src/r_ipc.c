#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include "engine_abi.h"

#define PIPE_BUF_SIZE 4096

struct RWorker {
    pid_t pid;
    int pipe_in[2];
    int pipe_out[2];
    char recv_buf[PIPE_BUF_SIZE];
    int timeout_ms;
    int is_alive;
};

/* ===================================================================== */
/* R AVAILABILITY PROBE                                                  */
/* ===================================================================== */

static int g_r_available = -1; /* -1 = unprobed, 0 = unavailable, 1 = available */

int r_worker_probe_availability(void) {
    if (g_r_available != -1) return g_r_available;
    
    g_r_available = 0;
    
    /* Check if R is installed */
    FILE* fp = popen("R --version >/dev/null 2>&1 && echo 'R_OK'", "r");
    if (!fp) return 0;
    
    char buffer[128];
    int r_ok = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, "R_OK")) {
            r_ok = 1;
            break;
        }
    }
    pclose(fp);
    
    /* Check if rayshader is available */
    if (r_ok) {
        fp = popen("Rscript -e 'library(rayshader)' >/dev/null 2>&1 && echo 'RAYSHADER_OK'", "r");
        if (fp) {
            int rayshader_ok = 0;
            while (fgets(buffer, sizeof(buffer), fp)) {
                if (strstr(buffer, "RAYSHADER_OK")) {
                    rayshader_ok = 1;
                    break;
                }
            }
            pclose(fp);
            if (rayshader_ok) g_r_available = 1;
        }
    }
    
    return g_r_available;
}

/* ===================================================================== */
/* STANDARD WORKER IMPLEMENTATION                                        */
/* ===================================================================== */

struct RWorker* r_worker_start(void) {
    if (!r_worker_probe_availability()) return NULL;

    struct RWorker* rw = (struct RWorker*)calloc(1, sizeof(struct RWorker));
    if (!rw) return NULL;
    
    rw->timeout_ms = 5000;
    rw->is_alive = 1;
    
    if (pipe(rw->pipe_in) < 0 || pipe(rw->pipe_out) < 0) {
        free(rw);
        return NULL;
    }
    
    rw->pid = fork();
    if (rw->pid < 0) {
        close(rw->pipe_in[0]); close(rw->pipe_in[1]);
        close(rw->pipe_out[0]); close(rw->pipe_out[1]);
        free(rw);
        return NULL;
    }
    
    if (rw->pid == 0) {
        close(rw->pipe_in[1]);
        close(rw->pipe_out[0]);
        dup2(rw->pipe_in[0], STDIN_FILENO);
        dup2(rw->pipe_out[1], STDOUT_FILENO);
        close(rw->pipe_in[0]);
        close(rw->pipe_out[1]);
        execlp("Rscript", "Rscript", "scripts/constraint_worker.R", "--persistent", (char*)NULL);
        _exit(127);
    }
    
    close(rw->pipe_in[0]);
    close(rw->pipe_out[1]);
    return rw;
}

void r_worker_stop(struct RWorker* rw) {
    if (!rw) return;
    if (rw->pid > 0) {
        write(rw->pipe_in[1], "quit\n", 5);
        kill(rw->pid, SIGTERM);
        waitpid(rw->pid, NULL, 0);
    }
    close(rw->pipe_in[1]);
    close(rw->pipe_out[0]);
    free(rw);
}

void r_worker_set_timeout(struct RWorker* rw, int timeout_ms) {
    if (rw) rw->timeout_ms = timeout_ms;
}

int r_worker_is_alive(struct RWorker* rw) {
    if (!rw) return 0;
    if (kill(rw->pid, 0) == 0) return 1;
    rw->is_alive = 0;
    return 0;
}

int r_worker_send(struct RWorker* rw, const char* cmd, const char* data) {
    if (!rw || !cmd) return -1;
    char buf[PIPE_BUF_SIZE];
    snprintf(buf, sizeof(buf), "%s\n%s\nEND\n", cmd, data ? data : "");
    ssize_t n = write(rw->pipe_in[1], buf, strlen(buf));
    return (n > 0) ? 0 : -1;
}

char* r_worker_recv(struct RWorker* rw) {
    if (!rw) return NULL;
    memset(rw->recv_buf, 0, sizeof(rw->recv_buf));
    struct pollfd pfd;
    pfd.fd = rw->pipe_out[0];
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, rw->timeout_ms);
    if (ret <= 0) return NULL;
    ssize_t n = read(rw->pipe_out[0], rw->recv_buf, sizeof(rw->recv_buf) - 1);
    return (n > 0) ? rw->recv_buf : NULL;
}

/* ===================================================================== */
/* STATISTICAL WORKER IMPLEMENTATION                                     */
/* ===================================================================== */

struct RWorker* r_stat_worker_start(void) {
    if (!r_worker_probe_availability()) return NULL;

    struct RWorker* rw = (struct RWorker*)calloc(1, sizeof(struct RWorker));
    if (!rw) return NULL;
    
    rw->timeout_ms = 30000;
    rw->is_alive = 1;
    
    if (pipe(rw->pipe_in) < 0 || pipe(rw->pipe_out) < 0) {
        free(rw);
        return NULL;
    }
    
    rw->pid = fork();
    if (rw->pid < 0) {
        close(rw->pipe_in[0]); close(rw->pipe_in[1]);
        close(rw->pipe_out[0]); close(rw->pipe_out[1]);
        free(rw);
        return NULL;
    }
    
    if (rw->pid == 0) {
        close(rw->pipe_in[1]);
        close(rw->pipe_out[0]);
        dup2(rw->pipe_in[0], STDIN_FILENO);
        dup2(rw->pipe_out[1], STDOUT_FILENO);
        close(rw->pipe_in[0]);
        close(rw->pipe_out[1]);
        execlp("Rscript", "Rscript", "scripts/ai_statistical_brain.R", (char*)NULL);
        _exit(127);
    }
    
    close(rw->pipe_in[0]);
    close(rw->pipe_out[1]);
    return rw;
}

void r_stat_worker_stop(struct RWorker* rw) {
    if (!rw) return;
    if (rw->pid > 0) {
        write(rw->pipe_in[1], "quit\n", 5);
        kill(rw->pid, SIGTERM);
        waitpid(rw->pid, NULL, 0);
    }
    close(rw->pipe_in[1]);
    close(rw->pipe_out[0]);
    free(rw);
}

int r_stat_send_test(struct RWorker* rw, const char* test_name, const char* json_data) {
    if (!rw || !test_name || !json_data) return -1;
    char buf[PIPE_BUF_SIZE * 4];
    snprintf(buf, sizeof(buf), "%s\n%s\n", test_name, json_data);
    ssize_t n = write(rw->pipe_in[1], buf, strlen(buf));
    return (n > 0) ? 0 : -1;
}

char* r_stat_recv_result(struct RWorker* rw) {
    if (!rw) return NULL;
    memset(rw->recv_buf, 0, sizeof(rw->recv_buf));
    struct pollfd pfd;
    pfd.fd = rw->pipe_out[0];
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, rw->timeout_ms);
    if (ret <= 0) return NULL;
    ssize_t n = read(rw->pipe_out[0], rw->recv_buf, sizeof(rw->recv_buf) - 1);
    return (n > 0) ? rw->recv_buf : NULL;
}
