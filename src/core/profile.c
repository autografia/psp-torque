#include "obd/profile.h"

#include <stddef.h>

const obd_poll_def PROFILE_QV_POLL[] = {
    { 0x0C, 100, 0 },   /* RPM */
    { 0x0B, 100, 0 },   /* MAP */
    { 0x0D, 200, 1 },   /* velocidad */
    { 0x05, 1000, 2 },  /* refrigerante */
    { 0x0F, 1000, 2 },  /* admision */
    { 0x33, 5000, 3 }   /* baro, si existe */
};

const int PROFILE_QV_POLL_COUNT =
    (int)(sizeof(PROFILE_QV_POLL) / sizeof(PROFILE_QV_POLL[0]));

const obd_poll_def PROFILE_ENGINE_POLL[] = {
    { 0x0C, 150, 0 },   /* RPM */
    { 0x0B, 150, 0 },   /* MAP */
    { 0x49, 200, 1 },   /* pedal del acelerador, si existe */
    { 0x11, 200, 1 },   /* mariposa */
    { 0x04, 250, 1 },   /* carga */
    { 0x0D, 500, 2 },   /* velocidad */
    { 0x06, 500, 2 },   /* STFT */
    { 0x05, 1000, 3 },  /* refrigerante */
    { 0x0F, 1000, 3 },  /* admision */
    { 0x07, 1000, 3 },  /* LTFT */
    { 0x42, 2000, 4 },  /* tension */
    { 0x5C, 2000, 4 },  /* aceite, si existe */
    { 0x33, 5000, 4 },  /* baro */
    { 0x2F, 5000, 4 }   /* nivel combustible, si existe */
};

const int PROFILE_ENGINE_POLL_COUNT =
    (int)(sizeof(PROFILE_ENGINE_POLL) / sizeof(PROFILE_ENGINE_POLL[0]));
