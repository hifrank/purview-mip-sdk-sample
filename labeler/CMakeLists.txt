cmake_minimum_required(VERSION 3.16)
project(mip-sdk-cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COMPILE_ONLY "Compile-only mode (skip linking — for header/syntax verification)" OFF)

# ── MIP SDK location ────────────────────────────────────────────────
# Set MIP_SDK_PATH to the extracted MIP SDK root.
# Supports two layouts:
#   1. NuGet / flat:   MIP_SDK_PATH/include/mip/...    (headers only)
#   2. Full SDK:       MIP_SDK_PATH/file/include/mip/... + bins/
# Example: cmake -DMIP_SDK_PATH=/path/to/mip_sdk ..
if(NOT MIP_SDK_PATH)
    if(DEFINED ENV{MIP_SDK_PATH})
        set(MIP_SDK_PATH $ENV{MIP_SDK_PATH})
    else()
        message(FATAL_ERROR
            "MIP_SDK_PATH not set. Download the MIP SDK from "
            "https://aka.ms/mipsdkbins and set MIP_SDK_PATH to the extracted directory.\n"
            "  cmake -DMIP_SDK_PATH=/path/to/mip_sdk ..")
    endif()
endif()

# Determine platform architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(MIP_ARCH "arm64")
else()
    set(MIP_ARCH "x64")
endif()

# ── Detect SDK layout ──────────────────────────────────────────────
if(EXISTS "${MIP_SDK_PATH}/include/mip/common_types.h")
    # NuGet / flat layout: all headers under include/
    set(MIP_INCLUDE_DIRS "${MIP_SDK_PATH}/include")
    # macOS SDK puts libs under bins/release/<arch>/, Linux uses bins/release/
    if(EXISTS "${MIP_SDK_PATH}/bins/release/${MIP_ARCH}")
        set(MIP_LIB_DIR "${MIP_SDK_PATH}/bins/release/${MIP_ARCH}")
    else()
        set(MIP_LIB_DIR "${MIP_SDK_PATH}/bins/release")
    endif()
    message(STATUS "MIP SDK layout: NuGet/flat (${MIP_SDK_PATH}/include)")
elseif(EXISTS "${MIP_SDK_PATH}/file/include/mip/common_types.h")
    # Full SDK layout: separate file/, protection/, upe/ directories
    set(MIP_INCLUDE_DIRS
        "${MIP_SDK_PATH}/file/include"
        "${MIP_SDK_PATH}/protection/include"
        "${MIP_SDK_PATH}/upe/include")
    set(MIP_LIB_DIR "${MIP_SDK_PATH}/file/bins/release/${MIP_ARCH}")
    message(STATUS "MIP SDK layout: Full SDK (${MIP_SDK_PATH})")
else()
    message(FATAL_ERROR "Cannot find MIP SDK headers at ${MIP_SDK_PATH}. "
        "Expected include/mip/common_types.h or file/include/mip/common_types.h")
endif()

# ── Source files ────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/action.cpp
    src/auth_delegate.cpp
    src/consent_delegate.cpp
    src/profile_observer.cpp
    src/file_handler_observer.cpp
)

if(COMPILE_ONLY)
    # Compile-only mode: build object library to verify code compiles against headers
    add_library(mip_sdk_cli_objs OBJECT ${SOURCES})
    target_include_directories(mip_sdk_cli_objs PRIVATE ${MIP_INCLUDE_DIRS} src/)
    message(STATUS "COMPILE_ONLY mode: will verify compilation without linking")
else()
    add_executable(mip-sdk-cli ${SOURCES})

    target_include_directories(mip-sdk-cli PRIVATE ${MIP_INCLUDE_DIRS} src/)
    target_link_directories(mip-sdk-cli PRIVATE ${MIP_LIB_DIR})

    # Link MIP SDK libraries
    target_link_libraries(mip-sdk-cli PRIVATE
        mip_file_sdk
        mip_upe_sdk
        mip_protection_sdk
    )

    # ── System dependencies ────────────────────────────────────────
    if(APPLE OR UNIX)
        find_package(CURL REQUIRED)
        find_package(OpenSSL REQUIRED)
        find_package(PkgConfig)
        target_link_libraries(mip-sdk-cli PRIVATE
            CURL::libcurl
            OpenSSL::SSL
            OpenSSL::Crypto
        )
        if(PkgConfig_FOUND)
            pkg_check_modules(GSF IMPORTED_TARGET libgsf-1)
            pkg_check_modules(GMIME IMPORTED_TARGET gmime-3.0)
            if(GSF_FOUND)
                target_link_libraries(mip-sdk-cli PRIVATE PkgConfig::GSF)
            endif()
            if(GMIME_FOUND)
                target_link_libraries(mip-sdk-cli PRIVATE PkgConfig::GMIME)
            endif()
        endif()
    endif()

    # Copy MIP SDK shared libraries next to the binary for runtime
    if(APPLE)
        set(MIP_DYLIB_GLOB "${MIP_LIB_DIR}/*.dylib")
    elseif(UNIX)
        set(MIP_DYLIB_GLOB "${MIP_LIB_DIR}/*.so*")
    elseif(WIN32)
        set(MIP_DYLIB_GLOB "${MIP_LIB_DIR}/*.dll")
    endif()

    file(GLOB MIP_SHARED_LIBS ${MIP_DYLIB_GLOB})
    foreach(lib ${MIP_SHARED_LIBS})
        add_custom_command(TARGET mip-sdk-cli POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${lib} $<TARGET_FILE_DIR:mip-sdk-cli>
        )
    endforeach()
endif()
