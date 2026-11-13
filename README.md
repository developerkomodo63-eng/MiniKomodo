# Komodo Synths (VST3)

Tres plugins de Developer Komodo en el mismo repositorio y con el mismo CMake. Los dos sintes vienen en **dos versiones**: **Lite** (para tocar en vivo) y **Full** (todo lo bárbaro), y además hay un plugin de teclados. Son 5 plugins:

| Plugin | Qué es |
|---|---|
| **MiniKomodo Lite** | Minimoog en versión directo: 3 osc, filtro ladder 24 dB, 2 envolventes, LFO, chorus, arpegiador, 6 voces / mono legato, 28 presets. CPU mínima. |
| **MiniKomodo** (Full) | Todo lo anterior + **hard sync**, **cross-mod**, filtro multimodo (LP24/LP12/BP/HP + drive), **LFO 2**, **matriz de modulación 4×** (8 fuentes → 7 destinos), stereo por voz, drive, **delay ping-pong al tempo**, **reverb**, **osciloscopio**, 16 voces, 33 presets. |
| **PolyKomodo Lite** | Juno/Prophet en versión directo: saw + pulso (PWM) + sub + ruido, HPF, 2 ADSR, LFO, chorus I/II, arpegiador, 6 voces, 38 presets. |
| **PolyKomodo** (Full) | Todo lo anterior + **OSC 2**, **unison de hasta 7 capas** con detune y stereo (Super Saw), filtro multimodo + drive, **LFO 2**, **matriz 4×**, drive, **delay**, **reverb**, **osciloscopio**, 8 voces, 43 presets. |
| **KeysKomodo** | Teclados tipo workstation (estilo Korg Triton), todo por síntesis, 8 voces: **piano eléctrico FM** (Rhodes, Wurly, DX, vibráfono, marimba, campanas: 2 pares FM + "tine" + pickup con bark), **órgano de tonewheels** (9 drawbars, percusión 2ª/3ª, key click), **teclas analógicas** (strings/ensemble, pads, brass, clav, harpsichord, farfisa) y **coro** (formantes a-e-i-o-u). Efectos: drive, tono, chorus / ensemble / phaser / tremolo suitcase / **rotary Leslie**, delay al tempo, reverb, osciloscopio. 33 presets. |

## Lo pensado para directo (todas las versiones)
- **Program Change MIDI**: cambia de preset desde tu teclado/controlador (el selector de la interfaz se actualiza solo).
- Botón **PANIC**: corta notas colgadas, arpegiador y colas de delay/reverb.
- **Interfaz redimensionable** (mantiene proporción): útil en notebooks chicas o pantallas 4K.
- Pitch bend (±2 st), mod wheel (vibrato), pedal de sustain, velocity y drift analógico.
- Arpegiador (Up, Down, Up/Down, Random, Played; 1-3 octavas; 1/4 a 1/32 y tresillos) sincronizado al tempo del host.

## Pensados para PCs muy modestas
- Voces fijas, sin asignación de memoria en el hilo de audio, sin oversampling y **latencia 0** (buffers de 32–64 muestras).
- Filtro, envolventes y LFO a "control rate" (cada 16 muestras) con rampas lineales: sin zipper noise.
- Sin `juce_dsp`, sin AVX (solo SSE2) y runtime de MSVC estático: no requiere Visual C++ Redistributable.
- Medido en Linux (1 núcleo, buffer 64): peor caso **Lite ~0.5 %**; **Full** con todas las voces, unison/chorus/delay/reverb al máximo ~1.7 %.
- La versión Full (matriz, FX) es solo estéreo; la Lite también acepta salida mono.

## Matriz de modulación (Full)
Fuentes: LFO 1, LFO 2, Env filtro, Env vol, Velocity, Key, Mod wheel, Random (por nota).
Destinos: Pitch, Corte, Reso, Volumen, Pan, y en Mini *Osc2 pitch / Osc3 pitch*, en Poly *PW / Detune*. Cantidad bipolar (-100 % a +100 %).

## Estructura
```
Source/Common/KomodoDsp.h      DSP base (envolvente, PolyBLEP, voces, arpegiador, chorus mono)
Source/Common/KomodoFx.h       Solo Full: filtro multimodo, matriz, chorus/delay estéreo, scope
Source/Common/KomodoUi.h       Barra de presets, osciloscopio, editor escalable
Source/PluginProcessor.*       MiniKomodo (Lite/Full con la macro KOMODO_FULL)
Source/PluginEditor.h
Source/PolyKomodo/*            PolyKomodo (Lite/Full)
Source/KeysKomodo/*            KeysKomodo (KeysDsp.h: seno, ensemble, phaser, tremolo, rotary)
```
`CMakeLists.txt` genera 5 targets: `MiniKomodo`, `MiniKomodoLite`, `PolyKomodo`, `PolyKomodoLite`, `KeysKomodo`.

## Compilación en GitHub
1. Sube el repositorio (conserva `.github/workflows`).
2. Pestaña **Actions** → **Build Komodo Synths VST3** → **Run workflow** (o `push` a `main`).
3. Descarga el artefacto **Komodo-Synths-Windows-VST3**: contiene los 5 zips (`MiniKomodo`, `MiniKomodoLite`, `PolyKomodo`, `PolyKomodoLite`, `KeysKomodo`).

## Compilación manual
Requiere CMake 3.22+, Visual Studio 2022 y JUCE en la carpeta `JUCE`:

```powershell
git clone --depth 1 https://github.com/juce-framework/JUCE.git JUCE
cmake -S . -B build -A x64
cmake --build build --config Release --target MiniKomodo_VST3
cmake --build build --config Release --target MiniKomodoLite_VST3
cmake --build build --config Release --target PolyKomodo_VST3
cmake --build build --config Release --target PolyKomodoLite_VST3
cmake --build build --config Release --target KeysKomodo_VST3
```
