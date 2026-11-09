cmake_minimum_required(VERSION 3.22)
cmake_policy(SET CMP0091 NEW)
project(NONIUS VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-Laufzeit fest einbinden, damit das Plugin ohne installiertes
# "Visual C++ Redistributable" lädt.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(juce)

enable_testing()

add_subdirectory(core)
add_subdirectory(tests)
add_subdirectory(tools/analyse)
add_subdirectory(tools/calibrate)
add_subdirectory(tools/reference)
add_subdirectory(tools/vocal)
add_subdirectory(probe)
add_subdirectory(plugin)
add_subdirectory(tools/preview)
