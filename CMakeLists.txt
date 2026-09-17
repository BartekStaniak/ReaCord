cmake_minimum_required(VERSION 3.16)
project(ReaCord VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine output binary name following REAPER convention
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            set(REACORD_OUTPUT_NAME "reaper_reacord-arm64")
        else()
            set(REACORD_OUTPUT_NAME "reaper_reacord64")
        endif()
    else()
        set(REACORD_OUTPUT_NAME "reaper_reacord32")
    endif()
elseif(APPLE)
    set(REACORD_OUTPUT_NAME "reaper_reacord")
else()
    # Linux
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(REACORD_OUTPUT_NAME "reaper_reacord-aarch64")
    else()
        set(REACORD_OUTPUT_NAME "reaper_reacord-x86_64")
    endif()
endif()

# Core sources
set(REACORD_SOURCES
    src/core/plugin_entry.cpp
    src/core/config.cpp
    src/core/config.hpp
    src/core/state_snapshot.hpp
    src/reaper/reaper_observer.cpp
    src/reaper/reaper_observer.hpp
    src/reaper/api_export.cpp
    src/discord/discord_ipc.cpp
    src/discord/discord_ipc.hpp
    src/discord/discord_protocol.hpp
    src/discord/rate_limiter.hpp
    src/ui/settings_dialog.cpp
    src/ui/settings_dialog.hpp
)

if(WIN32)
    list(APPEND REACORD_SOURCES
        src/ui/reacord.rc
    )
endif()

# Create shared library extension
add_library(reaper_reacord SHARED ${REACORD_SOURCES})

set_target_properties(reaper_reacord PROPERTIES
    OUTPUT_NAME "${REACORD_OUTPUT_NAME}"
    PREFIX ""
)

target_include_directories(reaper_reacord PRIVATE
    src
    src/core
    src/reaper
    src/discord
    src/ui
    deps/reaper-sdk/sdk
    deps/wdl/WDL
)

# Compiler definitions
target_compile_definitions(reaper_reacord PRIVATE
    REACORD_VERSION="1.0.0"
    REACORD_CLIENT_ID_DEFAULT="1462972195658534965"
)

if(WIN32)
    target_compile_definitions(reaper_reacord PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    target_link_libraries(reaper_reacord PRIVATE
        ws2_32
        comctl32
        advapi32
        user32
        gdi32
        shell32
    )
    set_target_properties(reaper_reacord PROPERTIES
        SUFFIX ".dll"
    )
elseif(APPLE)
    target_compile_options(reaper_reacord PRIVATE
        -fvisibility=hidden
        -Wall
        -Wno-unused-parameter
        -O3
    )
    set_target_properties(reaper_reacord PROPERTIES
        SUFFIX ".dylib"
        LINK_FLAGS "-undefined dynamic_lookup"
    )
else()
    # Linux
    target_compile_options(reaper_reacord PRIVATE
        -fvisibility=hidden
        -fPIC
        -Wall
        -Wno-unused-parameter
        -O3
    )
    target_link_libraries(reaper_reacord PRIVATE
        pthread
        dl
    )
    set_target_properties(reaper_reacord PROPERTIES
        SUFFIX ".so"
    )
endif()

# Optional install target pointing to local REAPER UserPlugins if found
if(WIN32 AND DEFINED ENV{APPDATA})
    set(REAPER_USERPLUGINS "$ENV{APPDATA}/REAPER/UserPlugins")
    if(EXISTS "${REAPER_USERPLUGINS}")
        install(TARGETS reaper_reacord DESTINATION "${REAPER_USERPLUGINS}")
    endif()
endif()
