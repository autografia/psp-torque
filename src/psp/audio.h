/*
 * audio.h - avisos sonoros de PSP Torque.
 *
 * Campaneo del tema INITIAL D. Usa chime.wav de la carpeta de la app si
 * existe; si no, reproduce un campaneo sintetizado. No toca el coche ni
 * envia nada por el puerto OBD.
 */

#ifndef PSP_AUDIO_H
#define PSP_AUDIO_H

/* Reserva el canal de audio y arranca el hilo. Devuelve 1 si funciona. */
int audio_init(void);

/* Detiene el hilo y libera el canal. */
void audio_shutdown(void);

/* Activa o desactiva el campaneo (se repite mientras este activo). */
void audio_chime(int on);

/* Silencio entre repeticiones del campaneo, en milisegundos. */
void audio_set_gap_ms(int ms);

#endif
