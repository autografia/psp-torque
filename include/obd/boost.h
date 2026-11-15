#ifndef OBD_BOOST_H
#define OBD_BOOST_H

#include <stdint.h>

typedef enum {
    BARO_NONE = 0,
    BARO_PID,     /* PID 0x33, presion barometrica real */
    BARO_KEY_ON,  /* captura de MAP con el motor parado */
    BARO_CONFIG   /* valor configurable, estimacion */
} baro_source;

typedef struct {
    int map_valid;
    float map_kpa;
    uint32_t map_ts_ms;

    int baro_valid;
    float baro_kpa;
    baro_source baro_source;
    uint32_t baro_ts_ms;

    int boost_valid;
    float boost_bar;
    int boost_is_estimate;
} boost_state;

void boost_init(boost_state *state, float fallback_kpa);

void boost_set_map(boost_state *state, float kpa, uint32_t ts_ms);

/* PID 0x33: tiene prioridad sobre la captura key-on y el fallback. */
void boost_set_baro_pid(boost_state *state, float kpa, uint32_t ts_ms);

/* Captura MAP como baro si el motor esta parado y es el inicio de sesion.
 * Devuelve 1 si se ha capturado. */
int boost_capture_key_on(boost_state *state,
                         float map_kpa,
                         int rpm_valid, float rpm,
                         int speed_valid, float speed_kph,
                         uint32_t ts_ms, uint32_t session_ms);

void boost_recompute(boost_state *state);

#endif
