# Estudio Técnico de Factibilidad: Portabilidad Nativa de NES / Famicom a WonderSwan Color

**Autor:** Equipo de Desarrollo de Software y Arquitectura de Sistemas  
**Fecha:** Octubre 2026  
**Objetivo:** Evaluar la viabilidad arquitectónica, técnica y de rendimiento para la traslación estática y desarrollo de ports nativos de juegos de Nintendo Entertainment System (NES / Famicom) hacia la consola portátil Bandai WonderSwan Color (WSC).

---

## Índice

1. [Resumen Ejecutivo](#1-resumen-ejecutivo)
2. [Comparativa Arquitectónica Base (Hardware vs Hardware)](#2-comparativa-arquitectónica-base)
3. [Factibilidad de Puertos Básicos (Mappers Estándar)](#3-factibilidad-de-puertos-básicos)
4. [Adaptación de Resolución y Geometría de Pantalla](#4-adaptación-de-resolución-y-geometría-de-pantalla)
5. [Coprocesadores Especiales (Gráficos, Matemáticas y Mappers Avanzados)](#5-coprocesadores-especiales)
6. [Subsistema de Audio y Expansiones Sonoras](#6-subsistema-de-audio-y-expansiones-sonoras)
7. [Sincronización de Refresco y Preservación de Velocidad (75.47 Hz vs 60.10 Hz)](#7-sincronización-de-refresco-y-preservación-de-velocidad)
8. [Explotación Estratégica de la Memoria RAM (2 KB vs 64 KB)](#8-explotación-estratégica-de-la-memoria-ram)
9. [Mejoras Cualitativas al Portar de NES a WSC](#9-mejoras-cualitativas-al-portar-de-nes-a-wsc)
10. [Conclusiones y Hoja de Ruta de Ingeniería](#10-conclusiones-y-hoja-de-ruta-de-ingeniería)

---

## 1. Resumen Ejecutivo

El traslado de software desde la Nintendo Entertainment System (NES) hacia la Bandai WonderSwan Color (WSC) mediante decompilación y recompilación estática es **altamente viable y ventajoso**.

A diferencia de lo que ocurre al intentar portar consolas más avanzadas, la WonderSwan Color supera a la NES en prácticamente todos los parámetros de hardware:
* Su procesador (**NEC V30MZ**) es entre **2.5 y 4 veces más rápido** por ciclo que el 6502 de la NES.
* Su memoria RAM interna (**64 KB**) es **32 veces superior** a la de la NES (2 KB).
* Su subsistema gráfico soporta el doble de sprites (128 vs 64), elimina el límite de 8 sprites por línea (admitiendo hasta 32) y ofrece una paleta nativa de **4096 colores RGB444** frente a los 64 colores fijos de NES.
* El soporte de paletas por tile de 8x8 píxeles (que en NES requería el chip especial MMC5) es una característica nativa de fábrica en WSC.

Los dos únicos desafíos de ingeniería residen en:
1. **La geometría de pantalla**: Adaptar la relación de aspecto de 256x240 (4:3) al formato panorámico de 224x144 (14:9) o vertical de 144x224 (Tate).
2. **La tasa de refresco**: Adaptar la frecuencia del panel LCD de la WonderSwan (75.47 Hz) al bucle de juego de la NES (60.10 Hz) mediante una cadencia matemática 4:5 o interpolación de movimiento.

---

## 2. Comparativa Arquitectónica Base

| Parámetro | Nintendo Entertainment System (NES) | Bandai WonderSwan Color (WSC) | Margen / Impacto en el Port |
| :--- | :--- | :--- | :--- |
| **CPU** | Ricoh 2A03 (MOS 6502 sin BCD) @ 1.789773 MHz | NEC V30MZ (CISC x86-16 con pipeline) @ 3.072 MHz | **Superávit masivo**. V30MZ ejecuta instrucciones en 1-2 ciclos con mayor IPC. |
| **Memoria RAM CPU** | 2 KB SRAM ($0000-$07FF) | 64 KB IRAM de alta velocidad (0 wait-states) | **32x más memoria**. Espacio para cachés, LUTs y búferes. |
| **Memoria VRAM** | 2 KB CIRAM interna (2 nametables) | Compartida en IRAM (hasta 16 KB VRAM dedicada) | WSC soporta dos planos completos de 256x256 píxeles. |
| **Resolución** | 256 x 240 píxeles (overscan a 256x224) | 224 x 144 px (Landscape) / 144 x 224 px (Tate) | Requiere ajuste de viewport horizontal y vertical. |
| **Formato de Tiles** | 8x8 píxeles, 2BPP (4 colores por tile) | 8x8 píxeles, 2BPP planar o 4BPP planar | Idéntico a nivel de bits en memoria planar. |
| **Sprites (OAM)** | 64 sprites (8x8 u 8x16), máx 8 por scanline | 128 sprites (8x8), máx 32 por scanline | Duplica objetos y elimina el parpadeo de sprites. |
| **Profundidad de Color**| 64 colores maestros (25 activos en pantalla) | 4096 colores (12-bit RGB444), 16 paletas de 16 colores | Fidelidad 100% y capacidad de colorización extendida. |
| **Audio** | APU 5 canales (2 Pulso, 1 Triángulo, 1 Ruido, 1 DPCM) | 4 canales wavetable (Voice RAM 4-bit) + Ruido LFSR | WSC sintetiza formas de onda arbitrarias de 16 muestras. |
| **Tasa de Refresco** | 60.0988 Hz (NTSC) / 50.007 Hz (PAL) | 75.4717 Hz | Ratio 4:5 casi exacto (~0.46% de diferencia). |
| **Entradas / Mando** | 8 botones lógicos (Cruceta, A, B, Select, Start) | 11 botones físicos (Pad X, Pad Y, A, B, Start) | Superávit de botones físicos para funciones QoL. |

---

## 3. Factibilidad de Puertos Básicos

### Mapeo de CPU (6502 a V30MZ)
El procesador V30MZ implementa el juego de instrucciones 80186 con un pipeline optimizado. A diferencia del 8086 clásico, la mayoría de instrucciones de registro a registro se resuelven en 1 o 2 ciclos de reloj.
* **Mapeo de Registros**:
  * Acumulador `A` (8 bits) $\rightarrow$ Registro `AL`.
  * Índices `X` e `Y` $\rightarrow$ Registros `BL` y `BH` (o `DL` / `DH`).
  * Puntero de Pila `S` $\rightarrow$ Puntero relativo dentro de la IRAM.
  * Registro de Estado `P` (`C`, `Z`, `I`, `D`, `V`, `N`) $\rightarrow$ Las banderas de CPU x86 (`CF`, `ZF`, `OF`, `SF`) se corresponden de manera directa, haciendo que las instrucciones condicionales (`BEQ`, `BNE`, `BCS`, `BCC`, `BMI`, `BPL`) se traduzcan en saltos de máquina nativos equivalentes (`JZ`, `JNZ`, `JC`, `JNC`, `JS`, `JNS`).
* **Presupuesto de Ciclos**:
  La NES genera aproximadamente 0.43 MIPS efectivos. El V30MZ a 3.072 MHz genera entre 2.2 y 2.8 MIPS. Esto deja un margen del **500% al 600% de potencia sobrante**, permitiendo que cualquier código traducido o envoltorio de hardware corra sin ralentizaciones.

### Mappers Comunes
* **NROM (Sin mapper: *Super Mario Bros.*, *Donkey Kong*, *Pac-Man*)**:
  Complejidad de mapeo nula. Las ROMs de 16 KB o 32 KB se cargan de forma contigua en los bancos de cartucho de WSC.
* **Mappers de Pestillo Simple (UxROM, CNROM, AxROM)**:
  La conmutación de bancos de 16 KB o 32 KB se resuelve mediante un puntero en una tabla de saltos del HAL.
* **Mappers con Temporizador de Scanline (MMC1, MMC3)**:
  Títulos avanzados como *Super Mario Bros. 3* o *Mega Man 3-6* dependen de interrupciones de línea de exploración para fijar la barra de estado en pantalla. WSC dispone por hardware de interrupción de coincidencia de scanline (`LYC`) e interrupción de retorno horizontal (`HBlank`), además de una capa física de ventana independiente en `Screen 2`.

---

## 4. Adaptación de Resolución y Geometría de Pantalla

El área visible de la NES es de **256 x 240 píxeles** (recortada comúnmente a 256 x 224 por los televisores CRT de la época). La WonderSwan Color ofrece **224 x 144 píxeles** en horizontal.

Existen cuatro soluciones arquitectónicas dependiendo del género del videojuego:

### A. Ventana con Tracking Dinámico de Cámara (Plataformas y Acción)
El motor de juego almacena el plano virtual de 256x240 en la memoria de video y desplaza la ventana visible de 224x144 centrada sobre el personaje principal. Al aproximarse a los extremos verticales, la cámara realiza un paneo suave con márgenes de anticipación (*look-ahead margins*). Esta es la técnica estándar empleada históricamente en títulos oficiales como *Super Mario Bros. Deluxe* en Game Boy Color.

### B. Separación de Interfaz (HUD) en Screen 2
WonderSwan Color posee dos planos de hardware independientes (`Screen 1` y `Screen 2`) con registros de recorte propios (`0x30 - 0x33`).
* `Screen 1`: Se reserva para el renderizado del mundo y los escenarios en scroll.
* `Screen 2`: Se configura como una ventana fija superior o inferior de 16 o 24 píxeles de altura donde residen los marcadores de vidas, puntaje y energía, liberando el área de juego sin superponer elementos opacos.

### C. Modo Orientación Vertical (Tate Mode 144 x 224)
Para títulos de disparos verticales (*shooters* como *1942*, *Xevious*, *Star Force*, *TwinBee*):
* La consola se sujeta rotada 90 grados en sentido antihorario.
* La resolución vertical de 224 píxeles de la pantalla coincide de forma exacta con la altura útil de 224 píxeles de los juegos de NES.
* El campo de juego se disfruta en formato arcade nativo, remapeando los controles al Pad Y (`Y1-Y4`) para movimiento y al Pad X (`X1-X4`) para botones de disparo.

### D. Paneo Suave en Pantallas Fijas (Juegos de Arena de 240 líneas)
En juegos de pantalla fija sin scroll (*Donkey Kong*, *Bubble Bobble*, *Balloon Fight*), donde el nivel completo abarca 240 líneas:
* Se implementa un paneo vertical automático reactivo a la posición vertical del protagonista, o bien un modo comprimido que redistribuye las plataformas a lo ancho de la pantalla panorámica.

---

## 5. Coprocesadores Especiales

Muchos cartuchos de Famicom/NES integraban chips auxiliares en su placa de circuito. A continuación se detalla su factibilidad:

### Nintendo MMC5: Gráficos Extendidos (ExGFX) y Multiplicador
Contrario a la creencia popular de que el MMC5 es el chip más complejo de portar, en WonderSwan Color **es el más afín de todo el ecosistema**:
1. **Atributos de Paleta por Tile de 8x8 (ExGFX)**:
   * En NES estándar, las paletas de fondo solo podían cambiarse en bloques de 16x16 píxeles. El MMC5 incorporó memoria RAM dedicada para asignar paletas de forma individual a cada tile de 8x8 píxeles.
   * En WonderSwan Color, **este es el comportamiento nativo de fábrica**. Cada celda del mapa de tiles almacena una palabra de 16 bits donde los bits 9-11 definen la paleta para ese tile de 8x8 sin necesidad de lógica especial.
2. **Multiplicador Matemático por Hardware ($5205 / $5206)**:
   * Empleado en juegos de estrategia de Koei (*Romance of the Three Kingdoms II*, *Nobunaga's Ambition II*).
   * El chip multiplicaba dos valores de 8 bits para entregar un resultado de 16 bits.
   * En WSC, el procesador V30MZ ejecuta multiplicaciones nativas en 16 bits mediante la instrucción de ensamblador `MUL`, resolviendo el cálculo en pocos ciclos de reloj.

### Chips con Detección Scanline (MMC3, VRC4, Sunsoft FME-7)
La detección de líneas para divisiones de pantalla se resuelve mediante los registros de coincidencia scanline `LYC` del hardware de WSC, que disparan interrupciones vectorizadas en la línea de visualización exacta.

---

## 6. Subsistema de Audio y Expansiones Sonoras

### El Desafío de los Canales
La NES base cuenta con 5 canales (2 Pulsos, 1 Triángulo, 1 Ruido, 1 DPCM). Al incorporar chips de sonido extendido, los cartuchos sumaban entre 1 y 8 canales adicionales, alcanzando hasta **13 o 14 canales simultáneos**.

La WonderSwan Color posee un generador digital de sonido con **4 canales físicos**.

### Soluciones de Ingeniería en WonderSwan Color

1. **Mezclador por Software en Canal 2 (Streaming PCM en Hyper Voice)**:
   * El Canal 2 de la WSC permite streaming directo de muestras PCM a Voice RAM.
   * El procesador V30MZ puede ejecutar una rutina de mezcla por software en coma fija a 12 kHz o 16 kHz sumando los canales secundarios. A 12 kHz, este proceso consume entre el 20% y el 28% de la CPU por cuadro, dejando más del 70% libre para el juego.
2. **Síntesis por Tabla de Ondas en Voice RAM**:
   * Cada canal de la WSC lee una tabla de 16 muestras de 4 bits. Esto permite cargar formas de onda complejas que en otros sistemas requerirían múltiples osciladores.

### Evaluación por Chip de Audio Especial

* **Konami VRC6 (*Akumajou Densetsu / Castlevania III JP*, *Madara*)**:
  * *Canales*: 2 Pulsos + 1 Diente de Sierra (8 canales totales con APU).
  * *Viabilidad*: **Muy Alta**. La onda de sierra se replica con precisión matemática cargando una rampa ascendente de 7 pasos en una tabla de Voice RAM. Los pulsos secundarios se procesan en el mezclador de software.
* **Namco 163 (*Megami Tensei II*, *Rolling Thunder*, *Final Lap*)**:
  * *Canales*: Hasta 8 canales de síntesis por tabla de ondas multiplexadas de 4 bits.
  * *Viabilidad*: **Muy Alta (Afinidad Arquitectónica Directa)**. El Namco 163 y el sintetizador del WonderSwan se basan exactamente en el mismo principio físico: muestras de 4 bits leídas cíclicamente. Los instrumentos se transfieren a WSC sin conversión de formato.
* **Sunsoft 5B (*Gimmick!*)**:
  * *Canales*: 3 canales de onda cuadrada / generador tipo AY-3-8910 (8 canales totales).
  * *Viabilidad*: **Alta**. Ondas cuadradas con envolvente asignadas a Voice RAM con suavizado armónico de transiciones para evitar saturación en el altavoz del WSC.
* **Famicom Disk System (FDS / RP2C33) (*Zelda*, *Metroid*, *Akumajou Dracula*)**:
  * *Canales*: 1 canal de tabla de ondas de 64 pasos (6 bits) con modulación de frecuencia (FM/LFO).
  * *Viabilidad*: **Muy Alta**. La onda de 64 pasos se cuantiza a los 16 pasos de Voice RAM, y el vibrato dinámico se modula alterando la frecuencia en cada cuadro de 60 Hz.
* **Konami VRC7 (*Lagrange Point*)**:
  * *Canales*: 6 canales de síntesis FM (Yamaha YM2413 / OPLL).
  * *Viabilidad*: **Media / Requiere Muestreo**. Calcular 6 canales de operadores FM por software en tiempo real saturaría cerca del 60% de la CPU. La estrategia recomendada es pre-renderizar los 15 instrumentos característicos del chip como tablas de ondas de 16 nibbles en Voice RAM, preservando la tímbrica original a costo cero de CPU.

---

## 7. Sincronización de Refresco y Preservación de Velocidad

### La Matemática de la Frecuencia (75.47 Hz vs 60.10 Hz)
La pantalla de la WonderSwan Color refresca a **75.4717 Hz**, mientras que la NES NTSC lo hace a **60.0988 Hz**.

La relación entre ambas tasas es:
$$\frac{60.0988}{75.4717} \approx 0.7963 \approx \frac{4}{5} \quad (0.8000)$$

Al aplicar una **cadencia de 4:5** (4 avances de lógica por cada 5 refrescos de pantalla):
$$75.4717 \times \frac{4}{5} = \mathbf{60.377\text{ Hz}}$$

La variación frente a la consola original es de apenas un **0.46%** (imperceptible en velocidad y tono musical).

### Estrategias de Implementación
1. **Cadencia 4:5 (Pacing Clásico)**:
   En 4 cuadros consecutivos se ejecutan la física, la lógica y el audio. En el 5º cuadro, la lógica se mantiene estática y la pantalla simplemente refresca el búfer actual. Físicas, saltos, temporizadores y música permanecen idénticos al cartucho original.
2. **Interpolación a 75 fps Reales**:
   La física se ejecuta a 60 Hz en memoria, pero el renderizador interpola la posición entre el cuadro anterior y el cuadro actual de los sprites. El juego se visualiza a 75 fotogramas por segundo reales con una fluidez superior a la de la NES original.
3. **Búfer Circular de Audio (*Ring Buffer*)**:
   Para evitar chasquidos o desincronización sonora entre los 60 Hz del motor de sonido y los 75 Hz del hardware, se emplea un búfer circular de 4 a 6 tramas en RAM que absorbe el desfase del 0.46%.

---

## 8. Explotación Estratégica de la Memoria RAM

La NES poseía solo **2 KB de RAM de CPU**, **2 KB de VRAM** y **256 bytes de OAM**. La WonderSwan Color cuenta con **64 KB de RAM interna (IRAM)** ultrarrápida.

Alojar el estado de la NES toma apenas **4.5 KB**, dejando **59.5 KB libres** para optimizaciones críticas:

```
Distribución Típica de la Memoria IRAM (64 KB) en un Port de NES a WSC:

0x0000 +-------------------------------------------------------+
       | Estado NES: 2 KB CPU RAM + 256 B OAM + Variables      | ( ~3 KB )
0x0C00 +-------------------------------------------------------+
       | Nametables y Búfer de Scroll Extendido (Screen 1 & 2) | ( 4 KB )
0x1C00 +-------------------------------------------------------+
       | Búfer de Interpolación de Sprites y Cámara            | ( 0.5 KB )
0x1E00 +-------------------------------------------------------+
       | Búfer Circular de Audio y Voice RAM                   | ( 2 KB )
0x2600 +-------------------------------------------------------+
       | Tablas de Búsqueda Rápida (LUTs: Seno, Gravedad, Dist)| ( 4 KB )
0x3600 +-------------------------------------------------------+
       | Caché de Tiles Planar Pre-convertidos                 | ( 16 KB )
0x7600 +-------------------------------------------------------+
       | Búfer de Nivel Descomprimido Completo                 | ( 16 KB )
0xB600 +-------------------------------------------------------+
       | Pila de C, Búferes del HAL y Ranura de Savestate      | ( ~18.5 KB )
0xFFFF +-------------------------------------------------------+
```

### Ventajas Derivadas del Uso de RAM
1. **Caché Planar de Tiles**: Los gráficos se pre-convierten al formato nativo planar de WSC en memoria RAM. Cero sobrecarga de CPU en tiempo de ejecución.
2. **Descompresión Total de Niveles**: Se elimina el límite de scroll unidireccional y los tirones de carga en mitad de la partida.
3. **Doble Búfer Anti-Tearing**: Se renderiza en un plano mientras se muestra el otro, eliminando cortes horizontales en pantalla.
4. **Físicas por Tablas de Consulta (LUTs)**: Las matemáticas complejas de colisión y trigonometría se resuelven en un solo ciclo de lectura de memoria (`MOV AL, [BX + SI]`).
5. **DPCM Limpio**: Las muestras de percusión se precargan decodificadas en RAM, eliminando las pausas de CPU por DMA del cartucho original.

---

## 9. Mejoras Cualitativas al Portar de NES a WSC

Un port estático a WonderSwan Color no es una simple traslación, sino una **versión mejorada (*Remaster*)**:

1. **Cero Parpadeo de Sprites (*Zero Sprite Flickering*)**:
   WSC admite hasta 32 sprites por línea horizontal (frente al límite estricto de 8 de la NES). En escenas con múltiples enemigos, proyectiles y explosiones, los sprites se mantienen opacos y sólidos sin parpadear.
2. **Eliminación de Glitches en los Bordes del Scroll**:
   El búfer extendido de 256x256 en WSC erradica las columnas y bloques de color corruptos en los bordes de la pantalla típicos de los juegos de NES.
3. **Fidelidad Cromática Superior**:
   Posibilidad de asignar paletas RGB444 enriquecidas con sombras y degradados a títulos monocromáticos o con paletas apagadas.
4. **Calidad de Audio Acústicamente Optimizada**:
   Aplicación de filtros de suavizado armónico de transiciones en las ondas cuadradas para eliminar la estridencia metálica en el altavoz portátil.
5. **Guardado en Cualquier Punto (*Savestates*)**:
   Almacenamiento de múltiples capturas instantáneas de partida en la memoria no volátil del cartucho.

---

## 10. Conclusiones y Hoja de Ruta de Ingeniería

### Veredicto Final
La viabilidad técnica para crear un ecosistema de ports nativos de NES a WonderSwan Color es **extraordinariamente alta**. 
* La CPU de destino (V30MZ) posee holgura computacional de sobra.
* La abundancia de memoria RAM (64 KB) resuelve de raíz los problemas históricos de rendimiento y artefactos de la NES.
* El soporte nativo de paletas por tile de 8x8 de WSC simplifica enormemente la traslación de juegos complejos con chips avanzados como MMC5.

### Hoja de Ruta Recomendada para un Pipeline `nes-to-wsc`

1. **Fase 1: Capa de Abstracción Base (HAL)**
   * Implementar la traducción de instrucciones de 6502 a C / V30MZ.
   * Configurar el gestor de memoria unificada (mapeando los 2 KB de NES en la IRAM de WSC).
   * Implementar el bucle de pacing con cadencia 4:5.
2. **Fase 2: Pipeline de Video y Ventana**
   * Transposición de patrones 2BPP a planar WSC.
   * Mapeo de paleta maestra NES de 64 colores a RGB444.
   * Configuración de la capa de ventana en `Screen 2` para alojar los HUDs de los juegos.
3. **Fase 3: Motor de Audio Wave RAM**
   * Configuración de tablas de Voice RAM para canales de pulso y onda triangular.
   * Asignación del canal 4 de ruido LFSR.
   * Implementación del búfer circular anti-desfase de 75 Hz.
4. **Fase 4: Adaptaciones de Mappers Especiales**
   * Integración de controladores para MMC1, MMC3 (Scanline IRQ) y MMC5 (ExGFX nativo).
   * Implementación de tablas Voice RAM para VRC6 (sierra) y Namco 163 (wavetables).
