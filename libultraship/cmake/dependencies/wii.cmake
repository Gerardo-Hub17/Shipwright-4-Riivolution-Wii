message(STATUS "Configuring libultraship for Nintendo Wii")

# Por ahora vamos a desactivar cosas que no existen o no necesitamos en Wii
set(INCLUDE_MPQ_SUPPORT OFF CACHE BOOL "" FORCE)

# Definiciones útiles
add_compile_definitions(
    PLATFORM_WII=1
    GEKKO=1
    SPDLOG_NO_THREAD_ID
    SPDLOG_NO_TLS
    STBI_NO_THREAD_LOCALS
)

# Aquí más adelante añadiremos find_package / link libraries específicas de Wii

## Gerardo