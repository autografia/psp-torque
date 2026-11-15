#ifndef OBD_PROFILE_H
#define OBD_PROFILE_H

#include <stdint.h>

#define PROFILE_ID "ALFA_MITO_QV_2010"
#define PROFILE_DISPLAY_NAME "MiTo QV 2010"

/* Umbrales provisionales: no son limites oficiales del vehiculo. */
#define PROFILE_REDLINE_RPM 6500.0f
#define PROFILE_TACH_MAX_RPM 7000.0f
#define PROFILE_BOOST_MIN_BAR (-1.0f)
#define PROFILE_BOOST_MAX_BAR 1.5f
#define PROFILE_ECT_WARN_C 105.0f
#define PROFILE_ECT_CRIT_C 115.0f
#define PROFILE_IAT_WARN_C 70.0f

/* Fallback de presion barometrica, marcado EST en la UI. */
#define PROFILE_BARO_FALLBACK_KPA 101.3f

typedef struct {
    uint8_t pid;
    uint16_t period_ms;
    uint8_t priority;  /* 0 = mas alta */
} obd_poll_def;

extern const obd_poll_def PROFILE_QV_POLL[];
extern const int PROFILE_QV_POLL_COUNT;

/* PID del bloque 01 que se sondean siempre (mariposa y carga). */
extern const obd_poll_def PROFILE_EXTRA_POLL[];
extern const int PROFILE_EXTRA_POLL_COUNT;

/* PID de diagnostico avanzado (trims, avance, O2, distancia con MIL). */
extern const obd_poll_def PROFILE_MOTOR_POLL[];
extern const int PROFILE_MOTOR_POLL_COUNT;

/* Periodos de los PID que solo se sondean si la mascara los declara
 * soportados. */
#define PROFILE_PID_MAF_PERIOD_MS   1000
#define PROFILE_PID_PEDAL_PERIOD_MS 1000
#define PROFILE_PID_VOLT_PERIOD_MS  10000
#define PROFILE_PID_OIL_PERIOD_MS   10000
#define PROFILE_PID_FUEL_PERIOD_MS  20000

/* FASE E2 + A: presupuesto de peticiones del enlace.
 *
 * Medido sobre un clon ELM327 v1.5 por WiFi: una peticion completa
 * (envio + respuesta del coche + hueco entre peticiones) ronda los
 * 55-83 ms segun cuanto hueco quede entre comandos. El suelo medido
 * es de ~12 peticiones/s y, con el hueco eliminado (FASE C), ~18.
 *
 * El presupuesto es 13 req/s: la cifra CONSERVADORA (la que se sostiene
 * aunque el ELM327 no tolere comandos seguidos) mas un pequeño margen.
 * La demanda en el caso peor queda en 12,05 req/s y en un coche normal
 * alrededor de 11. */
#define PROFILE_LINK_BUDGET_REQ_S 13.0f

extern const obd_poll_def PROFILE_ENGINE_POLL[];
extern const int PROFILE_ENGINE_POLL_COUNT;

#endif
