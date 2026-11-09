juce_add_console_app(NoniusVocal PRODUCT_NAME "nonius-vocal")
target_sources(NoniusVocal PRIVATE Main.cpp)
target_compile_definitions(NoniusVocal PRIVATE
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0)
target_link_libraries(NoniusVocal PRIVATE
    nonius_core
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(NoniusVocal PRIVATE /utf-8)
endif()
