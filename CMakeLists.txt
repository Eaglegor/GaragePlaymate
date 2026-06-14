cmake_minimum_required(VERSION 3.22)

project(GaragePlaymate VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GARAGEPLAYMATE_PORTABLE_BUILD "Portable build uses ./data/ default path" OFF)
option(GARAGEPLAYMATE_ENABLE_ASIO "Enable ASIO audio driver" ON)
option(GARAGEPLAYMATE_BUILD_TESTS "Build unit tests" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(config IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${config} config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/bin/${config})
endforeach()

include(cmake/FetchJuce.cmake)
include(cmake/CompilerWarnings.cmake)
include(cmake/AsioSdk.cmake)

juce_add_gui_app(GaragePlaymate
    PRODUCT_NAME "GaragePlaymate"
    COMPANY_NAME "GaragePlaymate"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
)

# Stub main() until TASK_03 replaces it with START_JUCE_APPLICATION.
set_target_properties(GaragePlaymate PROPERTIES WIN32_EXECUTABLE FALSE)

target_sources(GaragePlaymate PRIVATE src/main.cpp)

target_link_libraries(GaragePlaymate
    PRIVATE
        juce::juce_core
        juce::juce_events
        juce::juce_data_structures
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

garageplaymate_apply_warning_flags(GaragePlaymate)

target_compile_definitions(GaragePlaymate
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        GARAGEPLAYMATE_PORTABLE_BUILD=$<BOOL:${GARAGEPLAYMATE_PORTABLE_BUILD}>
)

garageplaymate_configure_asio(GaragePlaymate)
