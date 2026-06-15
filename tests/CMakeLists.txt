add_executable(GaragePlaymateTests
    core/test_song_yaml_parser.cpp
)

target_compile_features(GaragePlaymateTests PRIVATE cxx_std_23)

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
