cmake_minimum_required(VERSION 3.22)
project(MiniKomodo VERSION 1.0.0)

# Clona JUCE junto a este archivo:  git clone --depth 1 https://github.com/juce-framework/JUCE.git
add_subdirectory(JUCE)

juce_add_plugin(MiniKomodo
    COMPANY_NAME "Developer Komodo"
    PLUGIN_MANUFACTURER_CODE Dkmd
    PLUGIN_CODE Mkmg
    FORMATS VST3 Standalone
    PRODUCT_NAME "MiniKomodo"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(MiniKomodo PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.h)

target_compile_definitions(MiniKomodo PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MiniKomodo
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
