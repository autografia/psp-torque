#ifndef OBD_DISCOVERY_H
#define OBD_DISCOVERY_H

#include <stdint.h>
#include <stddef.h>

#define OBD_DISC_MAX_DATA 32
#define OBD_DISC_HEADER 16

/*
 * Una linea de respuesta del ELM327 con cabeceras activadas.
 *
 * Ejemplos:
 *   "7E8 06 41 00 BE 3E B8 11"
 *   "18DAF110 06 41 00 BE 3E B8 11"
 *   "18 DA F1 10 06 41 00 BE 3E B8 11"
 *   "41 00 BE 3E B8 11"              (sin cabecera)
 *   "0: 49 02 01 57 50 30 ..."       (prefijo multilinea de algunos clones)
 */
typedef struct {
    char header[OBD_DISC_HEADER]; /* "" si la linea no trae cabecera */
    uint8_t data[OBD_DISC_MAX_DATA];
    int data_len;
} obd_disc_line;

/* 1 si la linea contiene datos utiles, 0 si hay que ignorarla. */
int obd_disc_parse_line(const char *text, obd_disc_line *out);

/*
 * Devuelve el payload util de una trama y su longitud.
 * Quita el byte PCI de ISO-TP (single frame 0x0N, first frame 0x1N LL,
 * consecutive frame 0x2N). Si la trama no trae PCI, devuelve todos los
 * bytes. Devuelve 0 si no hay datos (p. ej. flow control).
 */
int obd_disc_payload(const obd_disc_line *line, const uint8_t **payload);

/* Convierte bytes ASCII imprimibles. Corta en el primer byte no imprimible
 * que aparezca despues de haber empezado el texto. */
void obd_disc_ascii(const uint8_t *data, int len, char *out, size_t out_size);

/* Mascara de 4 bytes en orden big-endian (como la devuelve OBD-II). */
uint32_t obd_disc_mask(const uint8_t *data, int len);

#endif
