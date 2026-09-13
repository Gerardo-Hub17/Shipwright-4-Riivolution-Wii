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
# En PC con devkitPro
git clone https://github.com/Gerardo-Hub17/Shipwright-4-Riivolution-Wii.git
cd Shipwright-4-Riivolution-Wii
git checkout wii-cmake-support

export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=$DEVKITPRO/devkitPPC

# Usar el script
bash scripts/wii/build.sh

# O manualmente
cmake -S . -B build-wii -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Wii.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-wii

---

## 🔗 Referencias del backend GX (Xash3D-FWGS `ref-gx`)

Antes de escribir `gfx_gx.cpp` en serio, la referencia viva es el backend GX
del port de Xash3D del mismo autor.

**Repo:** https://github.com/Gerardo-Hub17/xash3d-fwgs/tree/ref-gx

### Mapa de archivos → funcionalidad

| Archivo | Responsabilidad | Uso en SoH |
|---------|-----------------|------------|
| `ref/gx/gx_local.h` | Structs (`gltexture_t`), defines, tipos | Inspiración para `TextureInfoGX` |
| `engine/platform/ogc/vid_ogc.c:89-139` | Init de VI + GX + framebuffer | `GfxRenderingAPIGX::Init()` |
| `ref/gx/gx_context.c:36-48` | Vertex format + TEV base | `GfxRenderingAPIGX::StartFrame()` |
| `ref/gx/gx_image.c:111-127` | `GX_InitTexObj` + `GX_InitTexObjLOD` | `GfxRenderingAPIGX::UploadTexture()` |
| `ref/gx/gx_image.c:505-565` | Upload completo con `DCFlushRange` | `GfxRenderingAPIGX::UploadTexture()` |
| `ref/gx/gx_draw.c:42-56` | `GX_Begin`/`GX_End` con quads | `GfxRenderingAPIGX::DrawTriangles()` |
| `ref/gx/gx_alias.c:708-898` | Setup de TEV stages por shader | `GfxRenderingAPIGX::LoadShader()` |
| `ref/gx/gx_backend.c:180-186` | Ejemplo de TEV con MODULATE/PASSCLR | `CreateAndLoadNewShader()` |

### Patrones clave

**Inicialización (una sola vez, en `Init()`):**
```c
VIDEO_Init();
GX_Init(fifo, FIFO_SIZE);
GX_SetCopyClear(background, 0x00ffffff);
GX_SetViewport(...);
GX_SetScissor(...);
GX_SetDispCopySrc/Dst/Gamma(...);
GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
```

Vertex format (en StartFrame() o una vez):

```c
GX_ClearVtxDesc();
GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ, GX_F32, 0);
GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
GX_SetNumChans(1);
GX_SetNumTexGens(1);
```

Dibujo (en DrawTriangles()):

```c
GX_Begin(GX_TRIANGLES, GX_VTXFMT0, num_tris * 3);
for (cada vértice) {
    GX_Position3f32(x, y, z);
    GX_Color4u8(r, g, b, a);
    GX_TexCoord2f32(s, t);
}
GX_End();
```
