cmake_minimum_required(VERSION 3.15)
project(RekordisMasterLimiter VERSION 1.0.0)

add_subdirectory(JUCE)

juce_add_plugin(RekordisMasterLimiter
    COMPANY_NAME "Rekordis"
    PLUGIN_MANUFACTURER_CODE "Rkds"
    PLUGIN_CODE "RkL1"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    FORMATS VST3 AU
    PRODUCT_NAME "RekordisMasterLimiter"
    VST3_CATEGORIES Fx Mastering)

juce_generate_juce_header(RekordisMasterLimiter)

target_compile_features(RekordisMasterLimiter PRIVATE cxx_std_17)

target_compile_definitions(RekordisMasterLimiter PUBLIC
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

juce_add_binary_data(RekordisMasterLimiter_Assets
    HEADER_NAME "BinaryData.h"
    NAMESPACE "BinaryData"
    SOURCES
        Assets/Background.png
)

target_sources(RekordisMasterLimiter PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_link_libraries(RekordisMasterLimiter PRIVATE
    RekordisMasterLimiter_Assets
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_recommended_config_flags
)
