cmake_minimum_required(VERSION 3.22)

project(NrpnToCc VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE viene scaricato automaticamente (non serve clonarlo a mano).
# La prima configurazione impiega qualche minuto per il download.
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Definizione del plugin: SOLO VST3, tipo STRUMENTO (VSTi).
# In Cubase gli insert MIDI VST3 non sono supportati, ma a uno STRUMENTO
# la DAW invia tutto il MIDI della traccia: cosi' il plugin riceve gli NRPN,
# li converte in CC e restituisce MIDI in uscita (instradabile in Cubase).
# Nessuna copia automatica: il .vst3 resta in
#   build/NrpnToCc_artefacts/Release/VST3/NRPN to CC.vst3
# ---------------------------------------------------------------------------
juce_add_plugin(NrpnToCc
    COMPANY_NAME              "GPR Music Project"
    PLUGIN_MANUFACTURER_CODE  Anta
    PLUGIN_CODE               Nrcc
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         TRUE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    FORMATS                   VST3
    PRODUCT_NAME              "NRPN to CC")

target_sources(NrpnToCc PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(NrpnToCc PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# MSVC: interpreta i sorgenti come UTF-8 (così accenti e simboli come le frecce
# vengono compilati e mostrati correttamente).
target_compile_options(NrpnToCc PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(NrpnToCc
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
