#ifndef OBD_SCHEDULER_H
#define OBD_SCHEDULER_H

#include <stdint.h>

#define OBD_SCHED_MAX 24

typedef struct {
    uint8_t pid;
    uint16_t period_ms;
    uint8_t priority;      /* 0 = mas alta */
    uint32_t next_due_ms;
    uint8_t no_data_count;
    uint8_t supported;     /* 0 desconocido, 1 confirmado, 2 no soportado */
} obd_poll_item;

typedef struct {
    obd_poll_item items[OBD_SCHED_MAX];
    int count;
    int in_flight;         /* indice o -1 */
} obd_scheduler;

void obd_sched_init(obd_scheduler *sched);

int obd_sched_add(obd_scheduler *sched, uint8_t pid,
                  uint16_t period_ms, uint8_t priority);

/* Indice del siguiente PID a pedir, o -1 si no toca o hay uno en vuelo. */
int obd_sched_next(obd_scheduler *sched, uint32_t now_ms);

/* Marca el PID como enviado. */
void obd_sched_begin(obd_scheduler *sched, int idx, uint32_t now_ms);

/* Cierra la peticion: reprograma y cuenta NO DATA. */
void obd_sched_end(obd_scheduler *sched, int idx, uint32_t now_ms,
                   int got_data);

void obd_sched_set_supported(obd_scheduler *sched, int idx, int supported);

/* Marca todos los PID no soportados segun la mascara del bloque.
 * first_pid = 0x01, 0x21, 0x41... Devuelve cuantos ha cambiado. */
int obd_sched_apply_mask(obd_scheduler *sched, uint32_t mask,
                         uint8_t first_pid);

/* Reagenda todo (cambio de modo, reconexion). */
void obd_sched_reset_due(obd_scheduler *sched, uint32_t now_ms);

#endif
