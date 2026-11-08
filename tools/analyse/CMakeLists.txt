juce_add_console_app(NoniusAnalyse PRODUCT_NAME "nonius-analyse")
target_sources(NoniusAnalyse PRIVATE Main.cpp)
target_compile_definitions(NoniusAnalyse PRIVATE
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0)
target_link_libraries(NoniusAnalyse PRIVATE
    nonius_core
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(NoniusAnalyse PRIVATE /utf-8)
endif()
