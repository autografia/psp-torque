#ifndef OBD_ELM327_H
#define OBD_ELM327_H

#include <stdint.h>
#include <stddef.h>

#define ELM_MAX_DATA 8
#define ELM_MAX_RAW 64

typedef enum {
    ELM_PARSE_NONE = 0,  /* ruido, eco, linea informativa */
    ELM_PARSE_DATA,      /* respuesta valida de datos */
    ELM_PARSE_NO_DATA,   /* NO DATA */
    ELM_PARSE_ERROR,     /* ?, ERROR, BUS ERROR... */
    ELM_PARSE_UNABLE     /* UNABLE TO CONNECT */
} elm_parse_result;

typedef struct {
    uint8_t mode;  /* byte de respuesta: 0x41 para modo 01 */
    uint8_t pid;
    uint8_t data[ELM_MAX_DATA];
    int data_len;
    char raw[ELM_MAX_RAW];
} elm_frame;

elm_parse_result elm_parse_line(const char *line, elm_frame *out);

/* 1 si la trama responde exactamente al modo/PID pedidos. */
int elm_frame_matches(const elm_frame *frame, uint8_t mode, uint8_t pid);

/* Construye "010C" (modo y PID sin el bit 0x40). */
int elm_build_request(char *buf, size_t size, uint8_t mode, uint8_t pid);

#endif
