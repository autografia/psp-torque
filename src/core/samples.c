#include "obd/samples.h"

#include <string.h>

void obd_store_init(obd_sample_store *store)
{
    memset(store, 0, sizeof(*store));
}

void obd_store_set(obd_sample_store *store, uint8_t pid, float value,
                   obd_unit unit, uint32_t ts_ms, obd_source source)
{
    for (int i = 0; i < store->count; i++) {
        if (store->items[i].pid == pid) {
            store->items[i].value = value;
            store->items[i].unit = unit;
            store->items[i].ts_ms = ts_ms;
            store->items[i].source = (uint8_t)source;
            store->items[i].valid = 1;
            return;
        }
    }

    if (store->count >= OBD_SAMPLE_MAX) {
        return;
    }

    obd_sample *sample = &store->items[store->count++];
    sample->pid = pid;
    sample->value = value;
    sample->unit = unit;
    sample->ts_ms = ts_ms;
    sample->source = (uint8_t)source;
    sample->valid = 1;
}

void obd_store_invalidate(obd_sample_store *store, uint8_t pid,
                          uint32_t ts_ms)
{
    for (int i = 0; i < store->count; i++) {
        if (store->items[i].pid == pid) {
            store->items[i].valid = 0;
            store->items[i].source = OBD_SRC_UNAVAILABLE;
            store->items[i].ts_ms = ts_ms;
            return;
        }
    }
}

const obd_sample *obd_store_get(const obd_sample_store *store, uint8_t pid)
{
    for (int i = 0; i < store->count; i++) {
        if (store->items[i].pid == pid) {
            return &store->items[i];
        }
    }
    return 0;
}

uint32_t obd_store_newest_ts(const obd_sample_store *store)
{
    uint32_t newest = 0u;
    for (int i = 0; i < store->count; i++) {
        const obd_sample *s = &store->items[i];
        if (!s->valid) {
            continue;
        }
        if (newest == 0u || s->ts_ms > newest) {
            newest = s->ts_ms;
        }
    }
    return newest;
}

int obd_sample_stale(const obd_sample *sample, uint32_t now_ms,
                     uint32_t max_age_ms)
{
    if (sample == 0 || !sample->valid) {
        return 1;
    }
    return (uint32_t)(now_ms - sample->ts_ms) > max_age_ms;
}
