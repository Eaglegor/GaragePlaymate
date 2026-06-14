set(GARAGEPLAYMATE_ASIO_SDK_PATH "" CACHE PATH
    "Path to the Steinberg ASIO SDK root (directory containing common/iasiodrv.h)")

function(garageplaymate_configure_asio target)
    if(NOT GARAGEPLAYMATE_ENABLE_ASIO)
        return()
    endif()

    set(asio_sdk_root "")
    if(GARAGEPLAYMATE_ASIO_SDK_PATH)
        set(asio_sdk_root "${GARAGEPLAYMATE_ASIO_SDK_PATH}")
    elseif(DEFINED ENV{ASIO_SDK_PATH})
        set(asio_sdk_root "$ENV{ASIO_SDK_PATH}")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/third_party/asio-sdk/common/iasiodrv.h")
        set(asio_sdk_root "${CMAKE_SOURCE_DIR}/third_party/asio-sdk")
    endif()

    set(asio_include_dir "")
    if(asio_sdk_root)
        if(EXISTS "${asio_sdk_root}/common/iasiodrv.h")
            set(asio_include_dir "${asio_sdk_root}/common")
        elseif(EXISTS "${asio_sdk_root}/iasiodrv.h")
            set(asio_include_dir "${asio_sdk_root}")
        endif()
    endif()

    if(NOT asio_include_dir)
        message(FATAL_ERROR
            "GARAGEPLAYMATE_ENABLE_ASIO is ON but the Steinberg ASIO SDK was not found.\n"
            "Download it from https://www.steinberg.net/asiosdk (license acceptance required),\n"
            "then either:\n"
            "  - set -DGARAGEPLAYMATE_ASIO_SDK_PATH=<path-to-asio-sdk>\n"
            "  - set ASIO_SDK_PATH environment variable\n"
            "  - extract to third_party/asio-sdk/\n"
            "Or configure with -DGARAGEPLAYMATE_ENABLE_ASIO=OFF for WASAPI-only builds.")
    endif()

    target_include_directories(${target} PRIVATE "${asio_include_dir}")
    target_compile_definitions(${target} PRIVATE JUCE_ASIO=1)
endfunction()
