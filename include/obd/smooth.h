#ifndef OBD_SMOOTH_H
#define OBD_SMOOTH_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Suavizado exponencial con constante de tiempo fija.                */
/* ------------------------------------------------------------------ */

/* Constante de tiempo del filtro, en milisegundos.
 *
 * Es el retardo que introduce: el filtro alcanza el 63% del valor real
 * pasado un tau, sea cual sea la cadencia de muestreo.
 *
 * Con un factor de mezcla fijo (0,3) el retardo dependia de la
 * cadencia: 2,8 muestras hasta el 63%, es decir 2,1 s cuando el RPM
 * solo se refrescaba a 1,3 Hz. Ese era el retardo que se notaba en el
 * tacometro cuando se aceleraba. */
#define OBD_SMOOTH_TAU_MS 150.0f

typedef struct {
    float value;
    uint32_t last_ts;
    int initialized;
} obd_smooth;

/* Factor de mezcla para un intervalo dt entre dos muestras:
 * alpha = 1 - exp(-dt/tau). */
float obd_smooth_alpha(uint32_t dt_ms);

/* Reinicia el filtro: la siguiente muestra se toma tal cual. */
void obd_smooth_reset(obd_smooth *s);

/* Aplica el filtro a una muestra y devuelve el valor suavizado. */
float obd_smooth_apply(obd_smooth *s, float v, uint32_t ts);

#endif
