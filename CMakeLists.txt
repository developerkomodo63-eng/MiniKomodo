cmake_minimum_required(VERSION 3.22)

# Runtime de MSVC enlazado estaticamente: los plugins corren en PCs sin el Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(DevKomodoSynths VERSION 1.0.0)

# Clona JUCE junto a este archivo:  git clone --depth 1 https://github.com/juce-framework/JUCE.git
add_subdirectory(JUCE)

# Sin juce_dsp (no se usa) y sin AVX: binarios chicos que corren en cualquier CPU x64 con SSE2.
function(komodo_plugin NAME CODE SOURCES)
    juce_add_plugin(${NAME}
        COMPANY_NAME "Developer Komodo"
        PLUGIN_MANUFACTURER_CODE Dkmd
        PLUGIN_CODE ${CODE}
        FORMATS VST3 Standalone
        PRODUCT_NAME "${NAME}"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE)
    target_sources(${NAME} PRIVATE ${SOURCES} Source/Common/KomodoDsp.h)
    target_include_directories(${NAME} PRIVATE Source)
    target_compile_definitions(${NAME} PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(${NAME}
        PRIVATE juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
endfunction()

# Sinte monofonico/polifonico estilo Minimoog (3 osciladores)
komodo_plugin(MiniKomodo Mkmg
    "Source/PluginProcessor.cpp;Source/PluginProcessor.h;Source/PluginEditor.h")

# Sinte polifonico liviano estilo Juno (bass / lead / pad / brass, con presets)
komodo_plugin(PolyKomodo Plkm
    "Source/PolyKomodo/PluginProcessor.cpp;Source/PolyKomodo/PluginProcessor.h;Source/PolyKomodo/PluginEditor.h")
