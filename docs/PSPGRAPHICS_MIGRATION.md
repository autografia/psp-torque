# Migración de pspDebugScreen al pipeline GU (Fase 2)

## Resumen

Este documento describe cómo se ha pasado del dashboard de texto
(`pspDebugScreen`) a un dashboard 2D real de 480×272 usando el pipeline
gráfico GU de la PSP (`pspgu.h`). La implementación está en:

- `src/psp/gfx.c` / `src/psp/gfx.h`: inicialización GU, rectángulos,
  líneas, arcos, círculos y texto con fuente bitmap 8×8.
- `src/psp/font8x8.h`: fuente 8×8 de dominio público
  (Daniel Hepper / Marcel Sondaar, basada en la VGA de IBM).
- `src/psp/main.c`: layout QV/ENGINE/TELEMETRY y animación de arranque.

La app sigue siendo de **solo lectura**: la capa gráfica no envía nada
al vehículo.

## APIs reales del SDK

La versión anterior de este documento usaba nombres que **no existen**
en PSPSDK (`SCE_GU_*`, `pspfloat.h`, `sceDisplaySetBufSize`,
`sceDisplaySetDoubleBufferingEnabled`, `sceGuOrtho`). Los nombres
correctos son:

| Uso | API correcta |
|---|---|
| Inicializar GU | `sceGuInit()` / `sceGuTerm()` |
| Draw buffer | `sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512)` |
| Display buffer | `sceGuDispBuffer(480, 272, (void*)0x88000, 512)` |
| Offset 2D | `sceGuOffset(2048 - 480/2, 2048 - 272/2)` |
| Viewport | `sceGuViewport(2048, 2048, 480, 272)` |
| Scissor | `sceGuScissor(0, 0, 480, 272)` |
| Color buffer | `GU_PSM_8888` (valor 3), **no** `GU_COLOR` |
| Dibujar | `sceGuDrawArray(prim, vtype, count, indices, vertices)` |
| Blend | `sceGuBlendFunc(op, src, dst, srcfix, dstfix)` — 5 argumentos |
| Modo de pantalla | `sceDisplaySetMode(PSP_DISPLAY_MODE_LCD, 480, 272)` |
| Swap | `sceGuSwapBuffers()` (gestiona `sceDisplaySetFrameBuf`) |

Notas importantes:

- No existe `pspfloat.h`; para `sinf/cosf` basta `<math.h>` y enlazar `-lm`.
- Para proyección 2D **no** se usa `sceGuOrtho`; se usa
  `GU_TRANSFORM_2D` en el formato de vértice y el offset/viewport GU.
- `sceGuSwapBuffers()` ya añade la base de VRAM al framebuffer; no hay
  que llamar a `sceDisplaySetFrameBuf` a mano con un offset como
  `0x88000`.
- La GE lee los vértices directamente de memoria física, **sin pasar por
  la caché de datos de la CPU**. Si los vértices están en un array normal
  hay que hacer `sceKernelDcacheWritebackRange(vbuf, bytes)` justo antes
  de `sceGuDrawArray`; si no, la GE ve datos viejos y la pantalla queda
  negra aunque el `sceGuClear` sí funcione.
- La lista de comandos que se pasa a `sceGuStart` la convierte la propia
  biblioteca a la alias uncached (`| 0x40000000`), por eso los comandos
  no necesitan volcado manual.

## Inicialización usada

```c
sceGuInit();

sceGuStart(GU_DIRECT, list);
sceGuDrawBuffer(GU_PSM_8888, (void *)0, 512);
sceGuDispBuffer(480, 272, (void *)0x88000, 512);
sceGuOffset(2048 - 480 / 2, 2048 - 272 / 2);
sceGuViewport(2048, 2048, 480, 272);
sceGuScissor(0, 0, 480, 272);
sceGuDisable(GU_DEPTH_TEST);
sceGuDisable(GU_TEXTURE_2D);
sceGuDisable(GU_CULL_FACE);
sceGuEnable(GU_SCISSOR_TEST);
sceGuEnable(GU_BLEND);
sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
sceGuFinish();
sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

sceDisplaySetMode(PSP_DISPLAY_MODE_LCD, 480, 272);
sceDisplayWaitVblankStart();
sceGuDisplay(GU_TRUE);
```

Cada frame:

```c
sceGuStart(GU_DIRECT, list);
sceGuClearColor(0xFF050505);   /* ABGR */
sceGuClear(GU_COLOR_BUFFER_BIT);

/* ... acumular vértices 2D ... */
sceGuDrawArray(GU_TRIANGLES,
               GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,
               count, 0, vertices);

sceGuFinish();
sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
sceGuSwapBuffers();
sceDisplayWaitVblankStart();
```

## Formato de vértice

```c
typedef struct {
    unsigned int color;  /* ABGR: 0xAABBGGRR */
    float x, y, z;
} Vertex;
```

El orden en memoria es **color y después posición** (GU ordena los
atributos como textura, color, normal, posición). El color de GU es
ABGR, por eso `#C8102E` se escribe `0xFF2E10C8` (macro `GFX_RGB`).

## Texto sin atlas

En lugar de subir un atlas de texturas, `gfx.c` dibuja los píxeles
encendidos de una fuente 8×8 como rectángulos. Para reducir primitivas,
cada fila se agrupa en tramos horizontales (run-length). Un frame del
dashboard usa unos pocos miles de quads, muy por debajo de lo que la GE
puede mover a 60 fps.

Ventajas:

- Sin texturas, sin swizzle, sin CLUT.
- Sin dependencia de `pspDebugScreen` (no hay cajas negras de fondo).
- Escalado entero (`scale` 1, 2, 3, 5) sin pérdida.

Si en el futuro se quiere un atlas, `gfx_text` es el único punto a
cambiar.

## Layout implementado

### Pantalla QV

- Tacómetro circular de 0 a 7000 rpm (arco de 270°), zona roja desde
  6500 rpm, número de RPM en el centro.
- Velocidad grande en km/h.
- Barra de boost de −1.0 a +1.5 bar con cero centrado y badge `EST`.
- Tiles ECT / IAT / MAP ABS. Umbrales de color según
  `profiles/ALFA_MITO_QV_2010.md`.
- Barra superior: pestañas, estado WiFi/OBD, hora.
- Barra inferior: baro y fuente (`PID33`, `KEY`, `EST`), sesión y
  `SOLO LECTURA`.

### Pantalla ENGINE

Rejilla 4×2: MAP, BOOST, ECT, IAT, THROTTLE, LOAD, IGN ADV, VOLT.
Los PID no disponibles se muestran como `N/D`; no se inventan valores.

### Pantalla TELEMETRY

- Ring buffer de 180 muestras (RPM, BOOST, ECT) actualizado cada 250 ms.
- Series con huecos: si una muestra no es válida, la línea se corta; no
  se interpola a través de un `NO DATA`.
- Máximos y mínimos de la sesión.

## Animación de arranque

Se ejecuta después de conectar con el ELM327, antes del bucle de
sondeo: cuatro hojas (círculos con borde rojo), tallo, título
`QV TELEMETRY` y línea verde/roja. Dura 1,6 s y se puede saltar con
`X` o `START`.

## Controles

- `L`/`R` o `LEFT`/`RIGHT`: cambiar QV / ENGINE / TELEMETRY.
- `START`: terminar la sesión (se muestra el resumen de texto).
- `X`: saltar la animación.

## Migración incremental aplicada

1. Se recuperó y congeló la Fase 1 de texto como base estable.
2. Se añadió `gfx.c` con primitivas 2D y fuente bitmap.
3. Se sustituyó `draw_screen()` por el layout gráfico.
4. Se mantuvo `pspDebugScreen` solo para la configuración de red y el
   resumen final (antes y después de GU).
5. La animación y el dashboard usan exclusivamente GU.

## Pendiente / siguientes pasos

- Validar en la PSP real el tacto de la fuente 8×8 y el contraste.
- Añadir un atlas de fuente con antialiasing si se quiere el look
  tipográfico de `UI_QV.md`.
- Añadir transiciones animadas entre modos (fundido/desplazamiento).
- Añadir la pantalla de grabación CSV y el badge `SIM` si se incorpora
  el simulador.
