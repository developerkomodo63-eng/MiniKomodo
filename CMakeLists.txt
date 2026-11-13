cmake_minimum_required(VERSION 3.22)

# Runtime de MSVC enlazado estaticamente: los plugins corren en PCs sin el Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(DevKomodoSynths VERSION 1.0.0)

# Clona JUCE junto a este archivo:  git clone --depth 1 https://github.com/juce-framework/JUCE.git
add_subdirectory(JUCE)

# Sin juce_dsp (no se usa) y sin AVX: binarios chicos que corren en cualquier CPU x64 con SSE2.
# FULL = 1: version completa (unison, matriz de modulacion, FX, osciloscopio).  FULL = 0: version Lite para tocar en vivo.
function(komodo_plugin NAME PRODUCT CODE FULL SOURCES)
    juce_add_plugin(${NAME}
        COMPANY_NAME "Developer Komodo"
        PLUGIN_MANUFACTURER_CODE Dkmd
        PLUGIN_CODE ${CODE}
        FORMATS VST3 Standalone
        PRODUCT_NAME "${PRODUCT}"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE)
    target_sources(${NAME} PRIVATE ${SOURCES} Source/Common/KomodoDsp.h Source/Common/KomodoFx.h Source/Common/KomodoUi.h)
    target_include_directories(${NAME} PRIVATE Source)
    target_compile_definitions(${NAME} PUBLIC
        KOMODO_FULL=${FULL}
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(${NAME}
        PRIVATE juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
endfunction()

set(MINI_SRC "Source/PluginProcessor.cpp;Source/PluginProcessor.h;Source/PluginEditor.h")
set(POLY_SRC "Source/PolyKomodo/PluginProcessor.cpp;Source/PolyKomodo/PluginProcessor.h;Source/PolyKomodo/PluginEditor.h")

# Estilo Minimoog (3 osciladores)
komodo_plugin(MiniKomodo     "MiniKomodo"      Mkmg 1 "${MINI_SRC}")
komodo_plugin(MiniKomodoLite "MiniKomodo Lite" Mkml 0 "${MINI_SRC}")

# Estilo Juno / Prophet (bass / lead / pad / brass / keys / plucks)
komodo_plugin(PolyKomodo     "PolyKomodo"      Plkm 1 "${POLY_SRC}")
komodo_plugin(PolyKomodoLite "PolyKomodo Lite" Pllt 0 "${POLY_SRC}")

# Teclados estilo workstation (piano electrico FM, organo, strings/brass/clav, coro) - un solo plugin
komodo_plugin(KeysKomodo     "KeysKomodo"      Kykm 1 "Source/KeysKomodo/PluginProcessor.cpp;Source/KeysKomodo/PluginProcessor.h;Source/KeysKomodo/PluginEditor.h;Source/KeysKomodo/KeysDsp.h")
