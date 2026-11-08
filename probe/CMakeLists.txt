# Phase 0: "Nonius Probe" – schreibt nur mit, wie sich der Host (Ableton) verhält.
juce_add_plugin(NoniusProbe
    COMPANY_NAME "McQuartz"
    PLUGIN_MANUFACTURER_CODE Mcqz
    PLUGIN_CODE Npr0
    PRODUCT_NAME "Nonius Probe"
    FORMATS VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(NoniusProbe PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(NoniusProbe PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(NoniusProbe
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
