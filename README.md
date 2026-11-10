# MiniKomodo (VST3) - clon liviano de Minimoog

Requisitos: CMake 3.22+, compilador C++17 (Visual Studio 2022 en Windows).

    git clone --depth 1 https://github.com/juce-framework/JUCE.git
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

El VST3 queda copiado automaticamente a la carpeta de plugins del sistema
(COPY_PLUGIN_AFTER_BUILD). Tambien se genera una version Standalone para probar.
