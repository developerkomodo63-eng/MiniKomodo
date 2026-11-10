# MiniKomodo VST3

El repositorio está preparado para compilar MiniKomodo automáticamente mediante GitHub Actions.

## Compilación en GitHub

1. Sube estos archivos a la raíz del repositorio (la carpeta `.github/workflows` debe conservarse).
2. En GitHub, abre la pestaña **Actions**.
3. Ejecuta **Build MiniKomodo VST3** con **Run workflow**, o realiza un `push` a la rama `main`.
4. Cuando termine correctamente, descarga el artefacto **MiniKomodo-Windows-VST3** desde la ejecución.

El workflow descarga JUCE automáticamente, configura CMake y compila únicamente el formato VST3 para Windows.

## Compilación manual (opcional)

Requiere CMake 3.22+, Visual Studio 2022 y JUCE disponible en la carpeta `JUCE` junto a este archivo:

```powershell
git clone --depth 1 https://github.com/juce-framework/JUCE.git JUCE
cmake -S . -B build -A x64
cmake --build build --config Release --target MiniKomodo_VST3
```
