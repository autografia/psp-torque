/* PSP Torque — Fase 2: Dashboard gráfico QV (funciones de dibujo)
 *
 * Funciones que dibujan el layout QV definido en UI_QV.md usando
 * pspDebugScreen. main.c debe llamarlas desde su bucle principal.
 * No tiene main(); el entry point es main.c.
 */

#ifndef REDLINE
#define REDLINE (int)(PROFILE_REDLINE_RPM)
#endif

#define W 480
#define H 272
#define MARGIN 8

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "obd/boost.h"
#include "obd/pid.h"
#include "obd/profile.h"
#include "obd/samples.h"
#include "obd/scheduler.h"

/* -- Funciones de dibujo QV -- */

/* Barra superior: pestañas + conexión + hora */
void gui_bar_superior(int mode, int conn)
{
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("                  ");

    /* Pestañas QV | ENGINE | TELEMETRY */
    int px = MARGIN;
    if (mode == 0) {
        pspDebugScreenSetXY(px, 0); pspDebugScreenPrintf("> QV       "); px += 48;
    } else {
        pspDebugScreenSetXY(px, 0); pspDebugScreenPrintf("  QV       "); px += 48;
    }
    if (mode == 1) {
        pspDebugScreenSetXY(px, 0); pspDebugScreenPrintf("> ENGINE   ");
    } else {
        pspDebugScreenSetXY(px, 0); pspDebugScreenPrintf("  ENGINE   ");
    }
    px += 68;
    if (mode == 2) {
        pspDebugScreenSetXY(px, 0); pspDebugScreenPrintf("> TELEMETRY");
    } else {
        pspDebugScreenSetXY(px, 0); pspDebugScreenPrintf("  TELEMETRY");
    }

    /* Estado WiFi/ONLINE */
    pspDebugScreenSetXY(300, 0);
    if (conn == 2) { pspDebugScreenPrintf("ONLINE    "); }
    else if (conn == 1) { pspDebugScreenPrintf("CONECTANDO "); }
    else { pspDebugScreenPrintf("DESCONECT  "); }

    /* Hora */
    time_t now_c = time(NULL);
    char tstr[9];
    strftime(tstr, sizeof(tstr), "%H:%M", localtime(&now_c));
    pspDebugScreenSetXY(420, 0); pspDebugScreenPrintf("%s", tstr);
}

/* Área central: tacómetro, velocidad, boost, tiles ECT/IAT/MAP */
void gui_area_central(int rpm, int speed, float boost, int boost_est,
                      int ect, int iat, int map_kpa)
{
    /* Tacómetro */
    pspDebugScreenSetXY(MARGIN, 8);
    char tacho[32];
    snprintf(tacho, sizeof(tacho), "TACO: %3d rpm   [0-%d rpm]", rpm, REDLINE);
    pspDebugScreenPrintf("%s", tacho);

    /* Velocidad */
    pspDebugScreenSetXY(W - MARGIN - 80, 8);
    char spd[16];
    snprintf(spd, sizeof(spd), "%3d km/h", speed);
    pspDebugScreenPrintf("%s", spd);

    /* Boost */
    pspDebugScreenSetXY(MARGIN, 8 + 16);
    char boost_str[32];
    if (boost_est) { snprintf(boost_str, sizeof(boost_str), "BOOST: %s%.2f EST", boost >= 0 ? "+" : "", boost); }
    else           { snprintf(boost_str, sizeof(boost_str), "BOOST: %s%.2f",        boost >= 0 ? "+" : "", boost); }
    pspDebugScreenPrintf("%s", boost_str);

    /* Tiles ECT IAT MAP */
    pspDebugScreenSetXY(MARGIN, 24);
    char ect_str[16]; snprintf(ect_str, sizeof(ect_str), "ECT: %d C", ect); pspDebugScreenPrintf("%s", ect_str);

    pspDebugScreenSetXY(MARGIN + 130 + 6, 24);
    char iat_str[16]; snprintf(iat_str, sizeof(iat_str), "IAT: %d C", iat); pspDebugScreenPrintf("%s", iat_str);

    pspDebugScreenSetXY(MARGIN + 2 * (130 + 6), 24);
    char map_str[16]; snprintf(map_str, sizeof(map_str), "MAP: %d kPa", map_kpa); pspDebugScreenPrintf("%s", map_str);
}

/* Barra inferior: baro + sesión + aviso */
void gui_bar_inferior(boost_state *boost, uint32_t session_ms)
{
    int by = H - MARGIN - 20;

    /* Fuente baro */
    char baro_str[32];
    if (boost->baro_source == BARO_PID) { snprintf(baro_str, sizeof(baro_str), "BARO: PID 0x33 %.1f kPa", boost->baro_kpa / 100.0f); }
    else if (boost->baro_source == BARO_KEY_ON) { snprintf(baro_str, sizeof(baro_str), "BARO: KEY-ON %.1f kPa", boost->baro_kpa / 100.0f); }
    else                                    { snprintf(baro_str, sizeof(baro_str), "BARO: EST %.1f kPa", boost->baro_kpa / 100.0f); }
    pspDebugScreenSetXY(MARGIN, by); pspDebugScreenPrintf("%s", baro_str);

    /* Sesión */
    int s = (int)(session_ms / 1000);
    int m = s / 60; s %= 60;
    char ses_str[16]; snprintf(ses_str, sizeof(ses_str), "SESION %02d:%02d", m, s);
    pspDebugScreenSetXY(W - MARGIN - 80, by); pspDebugScreenPrintf("%s", ses_str);

    /* Aviso SOLO LECTURA */
    pspDebugScreenSetXY(W - MARGIN - 60, by); pspDebugScreenPrintf("SOLO LECTURA");
}