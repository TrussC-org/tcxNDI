# =============================================================================
# tcxNDI - NDI integration addon for TrussC
# =============================================================================

cmake_minimum_required(VERSION 3.16)
set(ADDON_NAME tcxNDI)

# -----------------------------------------------------------------------------
# Find NDI SDK (manual download required)
# -----------------------------------------------------------------------------

# Search paths by platform
# Note: These are the default installation paths from NDI SDK installers
if(APPLE)
    set(_NDI_SDK_SEARCH_PATHS
        "$ENV{NDI_SDK_DIR}"
        "/Library/NDI SDK for Apple"              # Default installer location
        "$ENV{HOME}/NDI SDK for Apple"
    )
    set(_NDI_LIB_NAME "libndi.dylib")
    set(_NDI_LIB_SUBDIR "lib/macOS")
elseif(WIN32)
    set(_NDI_SDK_SEARCH_PATHS
        "$ENV{NDI_SDK_DIR}"
        "C:/Program Files/NDI/NDI 6 SDK"          # NDI 6 default
        "C:/Program Files/NewTek/NDI 5 SDK"       # NDI 5 legacy
    )
    set(_NDI_LIB_NAME "Processing.NDI.Lib.x64.dll")
    set(_NDI_LIB_SUBDIR "Lib/x64")
else()  # Linux
    set(_NDI_SDK_SEARCH_PATHS
        "$ENV{NDI_SDK_DIR}"
        "/usr/local/NDI SDK for Linux"            # Manual install location
        "$ENV{HOME}/NDI SDK for Linux"
    )
    set(_NDI_LIB_NAME "libndi.so")
    set(_NDI_LIB_SUBDIR "lib/x86_64-linux-gnu")
endif()

# Find header
find_path(NDI_INCLUDE_DIR
    NAMES Processing.NDI.Lib.h
    PATHS ${_NDI_SDK_SEARCH_PATHS}
    PATH_SUFFIXES include Include
)

# Find library
find_library(NDI_LIBRARY
    NAMES ndi Processing.NDI.Lib.x64
    PATHS ${_NDI_SDK_SEARCH_PATHS}
    PATH_SUFFIXES lib Lib lib/x64 Lib/x64 lib/macOS
)

# Check if NDI SDK was found
if(NOT NDI_INCLUDE_DIR OR NOT NDI_LIBRARY)
    set(NDI_SDK_FOUND FALSE)
    message(WARNING
"
======================================================================
  NDI SDK not found - tcxNDI will be disabled
======================================================================

  tcxNDI requires the NDI SDK which must be downloaded manually.

  DOWNLOAD:
    https://ndi.video/download-ndi-sdk/

  INSTALLATION:
    Run the installer - it will install to the default location below.

  DEFAULT PATHS (set by NDI installer):
    macOS:   /Library/NDI SDK for Apple/
    Windows: C:/Program Files/NDI/NDI 6 SDK/
    Linux:   (manual install to /usr/local/NDI SDK for Linux/)

  CUSTOM PATH:
    cmake -DNDI_SDK_DIR=/your/custom/path ..

  Searched: ${_NDI_SDK_SEARCH_PATHS}
======================================================================
"
    )
else()
    set(NDI_SDK_FOUND TRUE)
    message(STATUS "tcxNDI: Found NDI SDK")
    message(STATUS "  Include: ${NDI_INCLUDE_DIR}")
    message(STATUS "  Library: ${NDI_LIBRARY}")
endif()

# -----------------------------------------------------------------------------
# Addon library
# -----------------------------------------------------------------------------

file(GLOB ADDON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB ADDON_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

if(NDI_SDK_FOUND)
    # NDI SDK available - create full library
    if(NOT ADDON_SOURCES)
        add_library(${ADDON_NAME} INTERFACE)
        target_include_directories(${ADDON_NAME} INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${NDI_INCLUDE_DIR}
        )
        target_link_libraries(${ADDON_NAME} INTERFACE
            TrussC
            ${NDI_LIBRARY}
        )
    else()
        add_library(${ADDON_NAME} STATIC ${ADDON_SOURCES} ${ADDON_HEADERS})
        target_include_directories(${ADDON_NAME} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${NDI_INCLUDE_DIR}
        )
        target_link_libraries(${ADDON_NAME} PUBLIC
            TrussC
            ${NDI_LIBRARY}
        )
    endif()

    # macOS: Set rpath for dylib
    if(APPLE)
        set_target_properties(${ADDON_NAME} PROPERTIES
            BUILD_RPATH "@executable_path"
            INSTALL_RPATH "@executable_path"
        )
    endif()
else()
    # NDI SDK not found - create stub INTERFACE library
    add_library(${ADDON_NAME} INTERFACE)
    target_include_directories(${ADDON_NAME} INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(${ADDON_NAME} INTERFACE TrussC)
endif()

add_library(tc::${ADDON_NAME} ALIAS ${ADDON_NAME})
