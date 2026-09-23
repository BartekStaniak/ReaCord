cmake_minimum_required(VERSION 3.16)
project(ReaCord VERSION 1.0.5 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    enable_language(OBJCXX)
endif()

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
    src/reaper/reaper_csurf.cpp
    src/reaper/reaper_csurf.hpp
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
elseif(APPLE)
    list(APPEND REACORD_SOURCES
        deps/wdl/WDL/swell/swell-modstub.mm
    )
    set_source_files_properties(deps/wdl/WDL/swell/swell-modstub.mm PROPERTIES LANGUAGE OBJCXX)
else()
    list(APPEND REACORD_SOURCES
        deps/wdl/WDL/swell/swell-modstub-generic.cpp
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
    deps/wdl/WDL/swell
)

# Compiler definitions
target_compile_definitions(reaper_reacord PRIVATE
    REACORD_CLIENT_ID_DEFAULT="1462972195658534965"
)

if(NOT WIN32)
    target_compile_definitions(reaper_reacord PRIVATE
        SWELL_PROVIDED_BY_APP
    )
endif()

if(WIN32)
    target_compile_definitions(reaper_reacord PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    if(MSVC)
        target_compile_options(reaper_reacord PRIVATE
            /GS          # Buffer Security Check
            /guard:cf    # Control Flow Guard
        )
        target_link_options(reaper_reacord PRIVATE
            /DYNAMICBASE # ASLR
            /NXCOMPAT    # Data Execution Prevention (DEP)
            /guard:cf    # Control Flow Guard (Linker)
        )
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            target_link_options(reaper_reacord PRIVATE /HIGHENTROPYVA)
        endif()
    endif()
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
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        -Wall
        -Wno-unused-parameter
        -O3
    )
    target_link_libraries(reaper_reacord PRIVATE
        "-framework Cocoa"
    )
    set_target_properties(reaper_reacord PROPERTIES
        SUFFIX ".dylib"
        LINK_FLAGS "-undefined dynamic_lookup"
    )
else()
    # Linux
    target_compile_options(reaper_reacord PRIVATE
        -fvisibility=hidden
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        -fPIC
        -Wall
        -Wno-unused-parameter
        -O3
    )
    target_link_options(reaper_reacord PRIVATE
        -Wl,-z,relro
        -Wl,-z,now
        -Wl,-z,noexecstack
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
