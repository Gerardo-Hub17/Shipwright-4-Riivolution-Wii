# Port de Ship of Harkinian a Nintendo Wii

Documentación del port en curso del motor **Ship of Harkinian** (SoH) a la
**Nintendo Wii** (PowerPC Gekko, GX, libogc). Rama activa: `wii-cmake-support`.

---

#estado actual 

| Componente | Estado |
|-----------|--------|
| Compilación en Termux (ARM64/Linux) |  Funcional |
| Esqueleto `wii.cmake` con libogc/libfat |  Listo |
| Backend `GfxRenderingAPIGX` (esqueleto) |  Listo |
| Detección `NintendoWii` en CMake |  Listo |
| Compilación con devkitPPC |  Pendiente (sin PC) |
| `GfxWindowBackendWii` (input/ventana) |  Por implementar |
| Implementación real de GX en `gfx_gx.cpp` |  Por implementar |
| Sistema de input (WPAD/PAD) |  Por implementar |

---

##  Objetivo

Portar SoH a Wii usando:
- **Backend gráfico:** GX nativo (no OpenGL)
- **Toolchain:** devkitPro + devkitPPC
- **API base:** libogc (Wii), libfat (SD/USB)
- **Referencia:** `ref-gx` de Xash3D-FWGS (port previo del autor)

---

## Trabajo realizado

### 1. Portabilidad en Termux (ARM64)

Para poder validar el código en un entorno Linux/ARM antes de saltar a devkitPPC.

**Archivos parcheados:**
- `libultraship/src/ship/Context.cpp` — guarda `__ANDROID__` con `TERMUX`
- `soh/include/functions.h` — elimina declaraciones `__assert` conflictivas con bionic
- `libultraship/cmake/dependencies/android.cmake` — `PATCH_COMMAND` para libzip
- `libultraship/CMakeLists.txt` — detección automática de Termux + flags `-D__THROW=`
- `libultraship/src/CMakeLists.txt` — `else()` con `OpenGL::GL` para desktop

### 2. Infraestructura Wii

**Archivos nuevos:**
- `libultraship/cmake/dependencies/wii.cmake` — detección de libogc/libfat, targets importados, definiciones para Gekko/PPC
- `libultraship/include/fast/backends/gfx_gx.h` — cabecera del backend GX
- `libultraship/src/fast/backends/gfx_gx.cpp` — esqueleto con todos los stubs de `GfxRenderingAPI`
- `scripts/wii/build.sh` — script de build para devkitPro

**Modificaciones en CMake:**
- `libultraship/CMakeLists.txt` — include de `wii.cmake` cuando `CMAKE_SYSTEM_NAME == "NintendoWii"`
- `libultraship/src/CMakeLists.txt` — enlaza `Wii::OGC` y `Wii::FAT`, define `ENABLE_GX`, omite `ENABLE_OPENGL`
- `libultraship/src/fast/CMakeLists.txt` — excluye `gfx_opengl`, `gfx_sdl2`, `gfx_metal`, `gfx_direct3d`, `gfx_dxgi` en Wii

**Modificaciones en código:**
- `libultraship/include/fast/Fast3dWindow.h` — nuevo valor `FAST3D_GX = 5` en `enum WindowBackend`
- `libultraship/src/fast/Fast3dWindow.cpp` — instancia condicional de `GfxRenderingAPIGX` bajo `#ifdef ENABLE_GX`

---

##  Compilación en Termux (validación ARM)

```bash
# Dependencias
pkg install git cmake ninja clang python3 pkg-config
pkg install x11-repo && pkg update
pkg install sdl2 sdl2-net glew libpng mesa-dev libglvnd-dev
pkg install xorgproto libx11 libxext libxrandr libxinerama libxcursor libxi libxfixes
pkg install libopus opusfile libvorbis libogg

# Configuración
cd ~/Shipwright-4-Riivolution-Wii
export CMAKE_PREFIX_PATH=$PREFIX
cmake -S . -B build-cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="-DTERMUX -D__THROW=" \
  -DCMAKE_CXX_FLAGS="-DTERMUX -D__THROW="

# Parche manual de libzip si es la primera vez
sed -i 's/zstd::libzstd_static/zstd::libzstd_shared/g' \
  build-cmake/_deps/libzip-src/CMakeLists.txt

# Build
cmake --build build-cmake -j$(nproc)
