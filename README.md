# Komodo Synths (VST3)

Dos sintetizadores livianos de Developer Komodo, en el mismo repositorio y con el mismo CMake:

| Plugin | Qué es |
|---|---|
| **MiniKomodo** | Estilo Minimoog: 3 osciladores, filtro ladder 24 dB, 2 envolventes, LFO (4 formas), chorus. Modo **Poly** (6 voces) o **Mono** legato con glide. 25 presets clásicos (bajos Moog, leads, brass, pads, strings, keys, plucks, FX). |
| **PolyKomodo** | Estilo Juno: saw + pulso (PWM) + sub + ruido, HPF, filtro 24 dB, 2 ADSR, LFO, chorus I/II, 6 voces, 35 presets (Bass, Lead, Pad, Brass, Keys, Pluck). |

Ambos responden a pitch bend (±2 st), mod wheel (vibrato) y pedal de sustain.

## Pensados para PCs muy modestas
- 6 voces fijas, sin asignación de memoria en el hilo de audio, sin oversampling y **latencia 0** (funcionan con buffers de 32–64 muestras).
- Filtro, envolventes y LFO a "control rate" (cada 16 muestras) con rampas lineales: sin zipper noise.
- Sin `juce_dsp`, sin AVX (solo SSE2) y runtime de MSVC estático: no requiere Visual C++ Redistributable.
- Medido en el peor caso (6 voces, chorus, resonancia): ~0.4 % de un núcleo.

## Estructura
```
Source/Common/KomodoDsp.h      DSP compartido (envolvente, chorus, PolyBLEP, asignación de voces)
Source/PluginProcessor.*       MiniKomodo
Source/PluginEditor.h
Source/PolyKomodo/*            PolyKomodo
```

## Compilación en GitHub
1. Sube el repositorio (conserva `.github/workflows`).
2. Pestaña **Actions** → **Build Komodo Synths VST3** → **Run workflow** (o `push` a `main`).
3. Descarga el artefacto **Komodo-Synths-Windows-VST3**: contiene `MiniKomodo-Windows-VST3.zip` y `PolyKomodo-Windows-VST3.zip`.

## Compilación manual
Requiere CMake 3.22+, Visual Studio 2022 y JUCE en la carpeta `JUCE`:

```powershell
git clone --depth 1 https://github.com/juce-framework/JUCE.git JUCE
cmake -S . -B build -A x64
cmake --build build --config Release --target MiniKomodo_VST3
cmake --build build --config Release --target PolyKomodo_VST3
```
