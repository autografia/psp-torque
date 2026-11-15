#ifndef OBD_SAMPLES_H
#define OBD_SAMPLES_H

#include <stdint.h>
#include "obd/pid.h"

#define OBD_SAMPLE_MAX 24

typedef enum {
    OBD_SRC_REAL = 0,
    OBD_SRC_ESTIMATED,
    OBD_SRC_SIMULATED,
    OBD_SRC_UNAVAILABLE
} obd_source;

typedef struct {
    uint8_t pid;
    float value;
    obd_unit unit;
    uint32_t ts_ms;
    uint8_t source;
    uint8_t valid;
} obd_sample;

typedef struct {
    obd_sample items[OBD_SAMPLE_MAX];
    int count;
} obd_sample_store;

void obd_store_init(obd_sample_store *store);

void obd_store_set(obd_sample_store *store, uint8_t pid, float value,
                   obd_unit unit, uint32_t ts_ms, obd_source source);

void obd_store_invalidate(obd_sample_store *store, uint8_t pid,
                          uint32_t ts_ms);

const obd_sample *obd_store_get(const obd_sample_store *store, uint8_t pid);

int obd_sample_stale(const obd_sample *sample, uint32_t now_ms,
                     uint32_t max_age_ms);

#endif
