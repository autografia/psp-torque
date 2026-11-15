#include "obd/smooth.h"

#include <math.h>

float obd_smooth_alpha(uint32_t dt_ms)
{
    if (dt_ms == 0u) {
        /* Sin intervalo conocido se asume que el dato es nuevo: se
         * toma tal cual para no quedarse con un valor viejo. */
        return 1.0f;
    }
    float t = (float)dt_ms / 1000.0f;
    return 1.0f - expf(-t / (OBD_SMOOTH_TAU_MS / 1000.0f));
}

void obd_smooth_reset(obd_smooth *s)
{
    s->value = 0.0f;
    s->last_ts = 0;
    s->initialized = 0;
}

float obd_smooth_apply(obd_smooth *s, float v, uint32_t ts)
{
    if (!s->initialized) {
        s->value = v;
        s->initialized = 1;
    } else {
        uint32_t dt = ts - s->last_ts;
        float a = obd_smooth_alpha(dt);
        s->value = s->value + (v - s->value) * a;
    }
    s->last_ts = ts;
    return s->value;
}
