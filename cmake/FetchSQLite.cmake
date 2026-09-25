include(FetchContent)

# sqlite.org amalgamation mirrored on GitHub; pinned to the 3.53.4 tag commit.
# SOURCE_SUBDIR points at a non-existent folder so the mirror's own CMakeLists is not used.
FetchContent_Declare(
    sqlite_amalgamation
    GIT_REPOSITORY https://github.com/rhuijben/sqlite-amalgamation.git
    GIT_TAG fa2905e70a3d9659cd219162d7f1ebfb3715a206
    SOURCE_SUBDIR do-not-build
)

FetchContent_MakeAvailable(sqlite_amalgamation)

add_library(sqlite3 STATIC ${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite_amalgamation_SOURCE_DIR})
target_compile_definitions(sqlite3
    PRIVATE
        SQLITE_DQS=0
        SQLITE_DEFAULT_FOREIGN_KEYS=1
        SQLITE_OMIT_LOAD_EXTENSION
)
set_target_properties(sqlite3 PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(sqlite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()
