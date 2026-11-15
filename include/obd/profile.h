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

extern const obd_poll_def PROFILE_ENGINE_POLL[];
extern const int PROFILE_ENGINE_POLL_COUNT;

#endif
