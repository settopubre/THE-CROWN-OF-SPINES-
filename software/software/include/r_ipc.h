#ifndef R_IPC_H
#define R_IPC_H

#ifdef __cplusplus
extern "C" {
#endif

struct RWorker;

/* Probes if R and rayshader are available on the system */
int r_worker_probe_availability(void);

/* Standard worker */
struct RWorker* r_worker_start(void);
void r_worker_stop(struct RWorker* rw);
void r_worker_set_timeout(struct RWorker* rw, int timeout_ms);
int r_worker_is_alive(struct RWorker* rw);
int r_worker_send(struct RWorker* rw, const char* cmd, const char* data);
char* r_worker_recv(struct RWorker* rw);

/* Statistical worker */
struct RWorker* r_stat_worker_start(void);
void r_stat_worker_stop(struct RWorker* rw);
int r_stat_send_test(struct RWorker* rw, const char* test_name, const char* json_data);
char* r_stat_recv_result(struct RWorker* rw);

#ifdef __cplusplus
}
#endif

#endif /* R_IPC_H */
