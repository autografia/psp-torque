/*
 * gfx.h - capa grafica minima sobre GU para PSP Torque (Fase 2).
 *
 * Todo se dibuja con primitivas 2D en un framebuffer de 480x272 a 32 bits.
 * El texto usa una fuente bitmap 8x8 de dominio publico (font8x8.h).
 *
 * Solo lectura: esta capa no toca el vehiculo, solo pinta.
 */

#ifndef PSP_GFX_H
#define PSP_GFX_H

#include <stdint.h>

#define GFX_W 480
#define GFX_H 272

/* GU_COLOR_8888 usa orden ABGR en memoria. */
#define GFX_RGB(r, g, b) \
    (0xFF000000u | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | \
     (uint32_t)(r))
#define GFX_RGBA(r, g, b, a) \
    (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | \
     ((uint32_t)(g) << 8) | (uint32_t)(r))

/*
 * Paleta activa (docs/UI_QV.md). Antes eran macros constantes; ahora son
 * campos de una estructura para poder cambiar de tema en tiempo de
 * ejecucion desde la ventana de ajustes. El resto del codigo no cambia.
 */
typedef struct {
    uint32_t bg;
    uint32_t panel;
    uint32_t panel2;
    uint32_t track;
    uint32_t text;
    uint32_t dim;
    uint32_t red;
    uint32_t green;
    uint32_t amber;
    uint32_t crit;
    uint32_t ok;
    uint32_t cyan;
} gfx_theme;

#define GFX_THEME_COUNT 14

/* Contraste: 0 = bajo, 1 = normal, 2 = alto. */
void gfx_theme_apply(int preset, int contrast);
const char *gfx_theme_name(int preset);
extern gfx_theme g_active_theme;

/* FASE F2: atenuacion del contenido cuando el dato envejece.
 *
 * 100 = normal. Por debajo, el contenido (textos, agujas, arcos) se
 * apaga mientras el marco (fondo, paneles) se mantiene: asi el panel
 * sigue leyendose pero se ve claramente que lo que hay dentro esta
 * viejo. */
extern int g_gfx_dim;

/* Aplica el factor de atenuacion a un color. */
uint32_t gfx_dimmed(uint32_t c);

#define QV_BG       (g_active_theme.bg)
#define QV_PANEL    (g_active_theme.panel)
#define QV_PANEL2   (g_active_theme.panel2)
#define QV_TRACK    (g_active_theme.track)
#define QV_TEXT     (gfx_dimmed(g_active_theme.text))
#define QV_DIM      (gfx_dimmed(g_active_theme.dim))
#define QV_RED      (gfx_dimmed(g_active_theme.red))
#define QV_GREEN    (gfx_dimmed(g_active_theme.green))
#define QV_AMBER    (gfx_dimmed(g_active_theme.amber))
#define QV_CRIT     (gfx_dimmed(g_active_theme.crit))
#define QV_OK       (gfx_dimmed(g_active_theme.ok))
#define QV_CYAN     (gfx_dimmed(g_active_theme.cyan))

int gfx_init(void);
void gfx_shutdown(void);

void gfx_frame_begin(void);
void gfx_frame_end(void);

/* Metricas del ultimo frame presentado (modo DEBUG). */
int gfx_vertex_count(void);
uint32_t gfx_last_wait_us(void);

void gfx_rect(float x, float y, float w, float h, uint32_t color);
void gfx_border(float x, float y, float w, float h, float t, uint32_t color);
void gfx_line(float x1, float y1, float x2, float y2, float t, uint32_t color);
void gfx_circle(float cx, float cy, float r, uint32_t color);
void gfx_arc(float cx, float cy, float r, float t,
             float start_deg, float end_deg, int segments, uint32_t color);

/* Oscurece (percent < 100) o aclara (percent > 100) un color,
 * manteniendo el canal alfa. */
uint32_t gfx_shade(uint32_t c, int percent);

int gfx_text(int x, int y, const char *s, uint32_t color, int scale);
int gfx_text_width(const char *s, int scale);
void gfx_text_center(int cx, int y, const char *s, uint32_t color, int scale);
void gfx_text_right(int rx, int y, const char *s, uint32_t color, int scale);

#endif
