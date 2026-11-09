# Mess-Kern: reines C++17 ohne JUCE, wird vom Test-Programm und später vom Plugin genutzt.
add_library(nonius_core STATIC
    src/dsp.cpp
    src/loudness.cpp
    src/track.cpp
    src/mix.cpp
    src/evaluate.cpp
    src/profile.cpp
    src/vocal.cpp)
target_include_directories(nonius_core PUBLIC include)
target_compile_features(nonius_core PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(nonius_core PRIVATE /W4 /utf-8)
else()
    target_compile_options(nonius_core PRIVATE -Wall -Wextra)
endif()
