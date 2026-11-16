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

/* FASE F3: periodo base de cada PID, en ms. Es el periodo que se usa
 * cuando el PID esta en la pantalla activa; fuera de pantalla se
 * multiplica por PROFILE_SLOW_FACTOR.
 *
 *  PID  base   que es
 *  0x0C   200  RPM (5 Hz)
 *  0x0B   400  MAP
 *  0x0D   800  velocidad
 *  0x11   800  mariposa
 *  0x49   800  pedal (posicion D)
 *  0x4A   800  pedal (posicion E)
 *  0x04  1500  carga
 *  0x05  3000  refrigerante
 *  0x0F  3000  admision
 *  0x06  1000  STFT
 *  0x07  1000  LTFT
 *  0x0E  1000  avance
 *  0x14  1000  O2-1
 *  0x15  1000  O2-2
 *  0x21  2000  distancia con MIL
 *  0x42  5000  voltaje
 *  0x5C 10000  aceite
 *  0x2F 10000  combustible
 *  0x33 30000  baro
 */

/* Multiplicador del periodo cuando el PID no esta en pantalla. */
#define PROFILE_SLOW_FACTOR 4

/* Tope del periodo efectivo. */
#define PROFILE_MAX_PERIOD_MS 30000u

/* Periodo base de un PID, o 0 si no esta en la tabla. */
uint16_t obd_profile_base_period(uint8_t pid);

/* Periodo efectivo: el base si 'visible' es distinto de 0; si no, el
 * base por PROFILE_SLOW_FACTOR, con tope en PROFILE_MAX_PERIOD_MS. */
uint16_t obd_profile_period(uint8_t pid, int visible);

/* 1 si el PID tiene periodo base conocido. */
int obd_profile_knows_pid(uint8_t pid);

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
