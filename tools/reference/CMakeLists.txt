juce_add_console_app(NoniusReferenz PRODUCT_NAME "nonius-referenz")
target_sources(NoniusReferenz PRIVATE Main.cpp)
target_compile_definitions(NoniusReferenz PRIVATE
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0)
target_link_libraries(NoniusReferenz PRIVATE
    nonius_core
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(NoniusReferenz PRIVATE /utf-8)
endif()
