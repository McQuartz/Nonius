# Entwickler-Werkzeug: spielt eine Audiodatei durch das Plugin (ohne DAW) und speichert
# ein Bild der Oberfläche. So lässt sich die Anzeige prüfen, ohne Ableton zu starten.
juce_add_console_app(NoniusPreview PRODUCT_NAME "nonius-preview")
target_sources(NoniusPreview PRIVATE
    Main.cpp
    ../../plugin/Source/PluginProcessor.cpp
    ../../plugin/Source/PluginEditor.cpp
    ../../plugin/Source/LiveAnalyser.cpp
    ../../plugin/Source/ProfileLoader.cpp)
target_include_directories(NoniusPreview PRIVATE ../../plugin/Source)
target_compile_definitions(NoniusPreview PRIVATE
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0)
target_link_libraries(NoniusPreview PRIVATE
    NoniusData
    nonius_core
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_recommended_config_flags)
if(MSVC)
    target_compile_options(NoniusPreview PRIVATE /utf-8)
endif()
