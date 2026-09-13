# ============================================================
#  libultraship - Nintendo Wii (Gekko / PowerPC)
# ============================================================
message(STATUS "Configuring libultraship for Nintendo Wii")

# ================== Platform Definitions ==================
add_compile_definitions(
    PLATFORM_WII=1
    GEKKO=1
    __wii__=1
    HW_RVL=1

    # spdlog: Wii no tiene TLS ni hilos reales en libogc
    SPDLOG_NO_THREAD_ID
    SPDLOG_NO_TLS

    # stb_image: desactivar thread locals
    STBI_NO_THREAD_LOCALS

    # No hay sistema de archivos estándar Linux/Mac
    STORMLIB_NO_AUTO_LINK
)

# ================== Cosas que NO aplican en Wii ==================
set(INCLUDE_MPQ_SUPPORT OFF CACHE BOOL "" FORCE)
set(USE_OPENGLES OFF CACHE BOOL "" FORCE)
set(USE_OPENGL OFF CACHE BOOL "" FORCE)
set(USE_DIRECT3D OFF CACHE BOOL "" FORCE)
set(USE_METAL OFF CACHE BOOL "" FORCE)
set(USE_SDL2 OFF CACHE BOOL "" FORCE)

# ================== FetchContent setup ==================
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ================== nlohmann-json ==================
find_package(nlohmann_json QUIET)
if (NOT ${nlohmann_json_FOUND})
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ================== tinyxml2 ==================
find_package(tinyxml2 QUIET)
if (NOT ${tinyxml2_FOUND})
    set(tinyxml2_BUILD_TESTING OFF)
    FetchContent_Declare(
        tinyxml2
        GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
        GIT_TAG 11.0.0
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(tinyxml2)
endif()

# ================== spdlog ==================
find_package(spdlog QUIET)
if (NOT ${spdlog_FOUND})
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.16.0
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# ================== libzip (con parche zstd compartido) ==================
find_package(libzip QUIET)
if (NOT ${libzip_FOUND})
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(BUILD_TOOLS OFF)
    set(BUILD_REGRESS OFF)
    set(BUILD_EXAMPLES OFF)
    set(BUILD_DOC OFF)
    set(BUILD_OSSFUZZ OFF)
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_Declare(
        libzip
        GIT_REPOSITORY https://github.com/nih-at/libzip.git
        GIT_TAG v1.11.4
        OVERRIDE_FIND_PACKAGE
        PATCH_COMMAND sed -i "s/zstd::libzstd_static/zstd::libzstd_shared/g" <SOURCE_DIR>/CMakeLists.txt
    )
    FetchContent_MakeAvailable(libzip)
    list(APPEND ADDITIONAL_LIB_INCLUDES ${libzip_SOURCE_DIR}/lib ${libzip_BINARY_DIR})
endif()

# ================== libogc (viene con devkitPro) ==================
find_path(LIBOGC_INCLUDE_DIR
    NAMES ogc/gu.h
    PATHS $ENV{DEVKITPRO}/libogc/include
    NO_DEFAULT_PATH
)
find_library(OGC_LIBRARY
    NAMES ogc
    PATHS $ENV{DEVKITPRO}/libogc/lib/wii
    NO_DEFAULT_PATH
)
find_library(FAT_LIBRARY
    NAMES fat
    PATHS $ENV{DEVKITPRO}/libogc/lib/wii
    NO_DEFAULT_PATH
)

if (NOT LIBOGC_INCLUDE_DIR OR NOT OGC_LIBRARY)
    message(FATAL_ERROR "libogc not found. Asegúrate de tener DEVKITPRO apuntando a /opt/devkitpro y wii-dev instalado.")
endif()

message(STATUS "libogc found at ${OGC_LIBRARY}")
message(STATUS "libfat found at ${FAT_LIBRARY}")

# ================== Enlaces de Wii ==================
# Estos targets los usará libultraship/src/CMakeLists.txt
add_library(Wii::OGC INTERFACE IMPORTED)
target_include_directories(Wii::OGC INTERFACE ${LIBOGC_INCLUDE_DIR})
target_link_libraries(Wii::OGC INTERFACE ${OGC_LIBRARY})

add_library(Wii::FAT INTERFACE IMPORTED)
target_include_directories(Wii::FAT INTERFACE ${LIBOGC_INCLUDE_DIR})
target_link_libraries(Wii::FAT INTERFACE ${FAT_LIBRARY})

# ================== Include dirs adicionales ==================
# libultraship hace uso de estos ADDITIONAL_LIB_INCLUDES en src/CMakeLists.txt
list(APPEND ADDITIONAL_LIB_INCLUDES ${LIBOGC_INCLUDE_DIR})

# ================== Información de build ==================
message(STATUS "Wii config:")
message(STATUS "  DEVKITPRO:    $ENV{DEVKITPRO}")
message(STATUS "  DEVKITPPC:    $ENV{DEVKITPPC}")
message(STATUS "  libogc:       ${OGC_LIBRARY}")
message(STATUS "  libfat:       ${FAT_LIBRARY}")
