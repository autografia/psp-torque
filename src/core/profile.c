#include "obd/profile.h"

#include <stddef.h>

/* FASE E2 + A: los periodos estan calibrados contra la capacidad real
 * del enlace ELM327 WiFi, no contra lo que el coche "deberia" dar.
 *
 * Medido en un viaje real (fase2.40): una peticion completa tarda ~83 ms
 * con el hueco entre comandos, es decir ~12 peticiones/s. La suma de
 * 1000/periodo de todas las tablas no puede pasar de
 * PROFILE_LINK_BUDGET_REQ_S; si lo hace, el planificador se satura y
 * TODOS los PID se refrescan mas tarde de lo pedido.
 *
 * Reparto (FASE A): lo que se mira en el tacometro va rapido, lo que
 * cambia despacio (temperaturas, trims, voltaje, baro, combustible) va
 * lento. El presupuesto se reparte en vez de repartir el ancho de banda
 * a partes iguales, que es lo que hacia que el RPM cayera a 1,3 Hz.
 */

const obd_poll_def PROFILE_QV_POLL[] = {
    { 0x0C, 250, 0 },   /* RPM */
    { 0x0B, 500, 0 },   /* MAP */
    { 0x0D, 800, 1 },   /* velocidad */
    { 0x05, 4000, 2 },  /* refrigerante */
    { 0x0F, 4000, 2 },  /* admision */
    { 0x33, 20000, 3 }  /* baro, si existe */
};

const int PROFILE_QV_POLL_COUNT =
    (int)(sizeof(PROFILE_QV_POLL) / sizeof(PROFILE_QV_POLL[0]));

const obd_poll_def PROFILE_EXTRA_POLL[] = {
    { 0x11, 1000, 1 },  /* mariposa (siempre disponible) */
    { 0x04, 2000, 1 }   /* carga */
};

const int PROFILE_EXTRA_POLL_COUNT =
    (int)(sizeof(PROFILE_EXTRA_POLL) / sizeof(PROFILE_EXTRA_POLL[0]));

const obd_poll_def PROFILE_MOTOR_POLL[] = {
    { 0x06, 12000, 3 }, /* STFT */
    { 0x07, 12000, 3 }, /* LTFT */
    { 0x0E, 12000, 3 }, /* avance */
    { 0x14, 12000, 3 }, /* O2-1 */
    { 0x15, 12000, 3 }, /* O2-2 */
    { 0x21, 12000, 3 }  /* distancia con MIL */
};

const int PROFILE_MOTOR_POLL_COUNT =
    (int)(sizeof(PROFILE_MOTOR_POLL) / sizeof(PROFILE_MOTOR_POLL[0]));

const obd_poll_def PROFILE_ENGINE_POLL[] = {
    { 0x0C, 250, 0 },   /* RPM */
    { 0x0B, 500, 0 },   /* MAP */
    { 0x49, 1000, 1 },  /* pedal del acelerador, si existe */
    { 0x11, 1000, 1 },  /* mariposa */
    { 0x04, 2000, 1 },  /* carga */
    { 0x0D, 800, 2 },   /* velocidad */
    { 0x06, 12000, 2 }, /* STFT */
    { 0x05, 4000, 3 },  /* refrigerante */
    { 0x0F, 4000, 3 },  /* admision */
    { 0x07, 12000, 3 }, /* LTFT */
    { 0x42, 10000, 4 }, /* tension */
    { 0x5C, 10000, 4 }, /* aceite, si existe */
    { 0x33, 20000, 4 }, /* baro */
    { 0x2F, 20000, 4 }  /* nivel combustible, si existe */
};

const int PROFILE_ENGINE_POLL_COUNT =
    (int)(sizeof(PROFILE_ENGINE_POLL) / sizeof(PROFILE_ENGINE_POLL[0]));
