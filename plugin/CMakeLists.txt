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
juce_add_binary_data(NoniusData SOURCES Resources/standard-profil.json)

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
