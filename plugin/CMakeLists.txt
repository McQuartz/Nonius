# Phase 2: das eigentliche NONIUS-Plugin (VST3). Misst nur, verändert den Ton nicht.
juce_add_plugin(Nonius
    COMPANY_NAME "McQuartz"
    PLUGIN_MANUFACTURER_CODE Mcqz
    PLUGIN_CODE Nns1
    PRODUCT_NAME "Nonius"
    FORMATS VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx Analyzer
    COPY_PLUGIN_AFTER_BUILD FALSE)

# Standard-Profil aus den Referenz-Songs, fest eingebaut.
# Eine Datei Dokumente/Nonius/standard-profil.json hat Vorrang (neues Profil ohne neuen Build).
set(NONIUS_DATA_FILES Resources/standard-profil.json)
# Vocal-Zielwerte aus den Referenz-Stems (tools/vocal), eingebaut sobald vorhanden.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/vocal-profil.json")
    list(APPEND NONIUS_DATA_FILES Resources/vocal-profil.json)
    set(NONIUS_HAS_VOCAL_PROFILE 1)
else()
    set(NONIUS_HAS_VOCAL_PROFILE 0)
endif()
juce_add_binary_data(NoniusData SOURCES ${NONIUS_DATA_FILES})
# Jedes Ziel, das NoniusData nutzt (Plugin, Vorschau), sieht, ob das Vocal-Profil eingebaut ist.
target_compile_definitions(NoniusData INTERFACE NONIUS_HAS_VOCAL_PROFILE=${NONIUS_HAS_VOCAL_PROFILE})

target_sources(Nonius PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/LiveAnalyser.cpp
    Source/ProfileLoader.cpp)

target_compile_definitions(Nonius PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(MSVC)
    target_compile_options(Nonius PRIVATE /utf-8)
endif()

target_link_libraries(Nonius
    PRIVATE
        NoniusData
        nonius_core
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
