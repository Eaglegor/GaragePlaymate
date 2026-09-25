add_executable(GaragePlaymateTests
    core/test_song_yaml_parser.cpp
    core/test_song_folder_scanner.cpp
    core/test_sqlite_store.cpp
    core/test_data_path_resolver.cpp
)

target_compile_features(GaragePlaymateTests PRIVATE cxx_std_20)

target_compile_definitions(GaragePlaymateTests
    PRIVATE
        TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

target_link_libraries(GaragePlaymateTests
    PRIVATE
        GaragePlaymateStorage
        Catch2::Catch2WithMain
)

garageplaymate_apply_warning_flags(GaragePlaymateTests)

include(CTest)
include(Catch)
catch_discover_tests(GaragePlaymateTests)

# Audio-layer tests need JUCE audio modules and a message manager, so they build
# as a separate JUCE console app that compiles the audio sources directly.
juce_add_console_app(GaragePlaymateAudioTests
    PRODUCT_NAME "GaragePlaymateAudioTests"
)

target_sources(GaragePlaymateAudioTests
    PRIVATE
        audio/main.cpp
        audio/test_audio_device_service.cpp
        ${GARAGEPLAYMATE_AUDIO_SOURCES}
)

target_include_directories(GaragePlaymateAudioTests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(GaragePlaymateAudioTests
    PRIVATE
        ${GARAGEPLAYMATE_JUCE_DEFINITIONS}
)

target_link_libraries(GaragePlaymateAudioTests
    PRIVATE
        GaragePlaymateCore
        juce::juce_core
        juce::juce_events
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        Catch2::Catch2
    PUBLIC
        juce::juce_recommended_config_flags
)

garageplaymate_apply_warning_flags(GaragePlaymateAudioTests)

catch_discover_tests(GaragePlaymateAudioTests)
