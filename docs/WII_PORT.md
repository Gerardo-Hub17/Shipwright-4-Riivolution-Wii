# Port de Ship of Harkinian a Nintendo Wii

Documentación del port en curso del motor **Ship of Harkinian** (SoH) a la
**Nintendo Wii** (PowerPC Gekko, GX, libogc). Rama activa: `wii-cmake-support`.

---

##  Estado actual

| Componente | Estado |
|-----------|--------|
| Compilación en Termux (ARM64/Linux) |  Funcional |
| `wii.cmake` con libogc + libfat + SDL2 |  Listo |
| Backend gráfico `GfxRenderingAPIGX` |  Implementado (Init, Upload, Draw, Shaders) |
| Backend ventana/input `GfxWindowBackendWii` |  Implementado (WPAD/PAD, timing, eventos) |
| Scancodes Wii en el sistema de mapping |  Listo |
| Mappings por defecto para mandos Wii |  Listo |
| Audio vía SDL2 |  Listo (sin cambios de código) |
| Sistema de archivos (`fatInitDefault`) |  Implementado |
| `MALLOC_MEM2` (heap en MEM2) |  Implementado |
| Big-endian |  Automático (`__BYTE_ORDER__`) |
| Protección `__wii__` en `Fast3dGui` |  Implementado |
| Filtros SDL del controller en Wii |  Implementado |
| Compilación con devkitPPC |  Pendiente (sin PC) |
| TEV stages avanzados |  Por completar cuando compile |
| `GetPixelDepth` (efecto escudo) |  Por implementar |
| Framebuffers dinámicos |  Por implementar |

---

##  Objetivo

Portar SoH a Wii usando:
- **Backend gráfico:** GX nativo (no OpenGL)
- **Toolchain:** devkitPro + devkitPPC
- **API base:** libogc (Wii), libfat (SD/USB)
- **Referencia:** `ref-gx` de Xash3D-FWGS (port previo del autor)

---

## 🗺️ Arquitectura del port

```

Fast3dWindow   Fast3dGui        AudioPlayer
│               │                │
▼               ▼                ▼
GfxRendering    ImGui          SDLAudioPlayer
APIGX          (sin SDL)      (SDL2-wii)
│
▼
GfxWindowBackendWii
│
├── WPAD_ScanPads() → Wiimote / Nunchuk / Classic
└── PAD_ScanPads()  → GameCube

```

- **Gráficos**: `GfxRenderingAPIGX` traduce el modelo de render de OpenGL (shaders, VBO, texturas) a GX (TEV stages, `GX_Begin`, `GX_TexObj`).
- **Input**: `GfxWindowBackendWii` lee WPAD/PAD cada frame y emite callbacks de teclado con scancodes Wii (1000+).
- **Mapping**: el sistema de control de SoH traduce esos scancodes a botones N64 (`BTN_A`, `BTN_B`, etc.).
- **Audio**: SDL2 (que devkitPro provee para Wii) se encarga de todo.
- **Archivos**: libfat monta SD/USB y SoH busca assets en `sd:/apps/soh`.

---

##  Componentes implementados

### 1. `libultraship/cmake/dependencies/wii.cmake`

Detección de devkitPro, libogc y libfat. Define:
- Targets importados `Wii::OGC` y `Wii::FAT`.
- Flags de compilación: `PLATFORM_WII`, `GEKKO`, `__wii__`, `HW_RVL`.
- `SPDLOG_NO_TLS` y `SPDLOG_NO_THREAD_ID` (libogc no tiene TLS real).
- `STBI_NO_THREAD_LOCALS`.
- `USE_SDL2` habilitado (audio + timing).
- `find_package(SDL2 REQUIRED)`.
- Detección opcional de `libstdc++fs` para `std::filesystem`.

### 2. Backend gráfico GX

**`libultraship/include/fast/backends/gfx_gx.h`** y **`gfx_gx.cpp`**.

Implementa `GfxRenderingAPIGX` (hereda de `GfxRenderingAPI`, ~45 métodos).

**Métodos reales:**
- `Init()` — `VIDEO_Init` + `GX_Init` + `GX_SetCopyClear` + `GX_SetViewport` + FIFO.
- `EndFrame()` — `GX_DrawDone` + flip de framebuffer + `VIDEO_WaitVSync`.
- `UploadTexture()` — convierte RGBA32 entrelazado a `GX_TF_RGBA8` (tiles 4×4, dos planos RA y GB).
- `DrawTriangles()` — emite vértices con `GX_Begin`/`GX_Position3f32`/`GX_Color4u8`/`GX_TexCoord2f32` leyendo el VBO con offsets reales.
- `CreateAndLoadNewShader()` — analiza `CCFeatures` y calcula offsets del VBO + TEV stages mínimos.
- `LoadShader()` — configura TEV (MODULATE con textura, PASSCLR sin textura).
- Pool de shaders (`mShaderProgramPool`) reutiliza shaders por par de IDs.

**Stubs por completar:**
- `GetPixelDepth` (efecto escudo/daño).
- Framebuffers dinámicos (`StartDrawToFramebuffer`, `CopyFramebuffer`).
- TEV de 2 ciclos (`opt_2cyc`), alpha blending complejo, masks, blend de 2 texturas.

### 3. Backend ventana/input `GfxWindowBackendWii`

**`libultraship/include/fast/backends/gfx_window_wii.h`** y **`gfx_window_wii.cpp`**.

- `Init()` — `WPAD_Init()` + `PAD_Init()`.
- `HandleEvents()` — `WPAD_ScanPads()` + `PAD_ScanPads()`, detecta Classic Controller vía `WPAD_Probe`, emite callbacks de teclado.
- `GetTime()` — `ticks_to_millisecs(gettime())`.
- `SwapBuffersBegin/End()` — no-op (lo hace `GfxRenderingAPIGX::EndFrame`).
- Stubs para mouse, resize, fullscreen (no aplican en Wii).

### 4. Scancodes y mappings Wii

**`libultraship/include/ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h`**

Añadidos scancodes Wii al enum `KbScancode` con valores 1000-1043:
```

LUS_KB_WII_A, LUS_KB_WII_B, LUS_KB_WII_X, LUS_KB_WII_Y,
LUS_KB_WII_START, LUS_KB_WII_SELECT, LUS_KB_WII_HOME,
LUS_KB_WII_DPAD_UP/DOWN/LEFT/RIGHT,
LUS_KB_WII_L, LUS_KB_WII_R, LUS_KB_WII_ZL, LUS_KB_WII_ZR,
LUS_KB_WII_STICK_UP/DOWN/LEFT/RIGHT,
LUS_KB_WII_STICK2_UP/DOWN/LEFT/RIGHT

```

**`LUS::ControllerDefaultMappings::SetDefaultKeyboardKeyToButtonMappings`**

Bloque `#ifdef __wii__` con mappings por defecto:
- `BTN_A` → Wiimote A / GC A / Classic A
- `BTN_B` → Wiimote B / GC B / Classic B
- `BTN_START` → Wiimote Plus / GC Start
- `BTN_L` → Nunchuk C / GC L
- `BTN_R` → Nunchuk Z / GC R
- `BTN_Z` → Classic ZR / GC Z
- `BTN_CUP/DOWN/LEFT/RIGHT` → stick derecho
- `BTN_DUP/DOWN/LEFT/RIGHT` → D-pad

### 5. Filtros CMake para Wii

**`libultraship/src/ship/CMakeLists.txt`**:
- Excluye `controller/controldevice/controller/mapping/sdl/` en Wii.
- Excluye `physicaldevice/SDLAddRemoveDeviceEventHandler` y `ConnectedPhysicalDeviceManager`.

**`libultraship/src/fast/CMakeLists.txt`**:
- Excluye `gfx_opengl`, `gfx_sdl2`, `gfx_metal`, `gfx_direct3d`, `gfx_dxgi`.

**`libultraship/src/CMakeLists.txt`**:
- `NintendoWii` enlaza `Wii::OGC` y `Wii::FAT` en lugar de `OpenGL::GL`.
- Define `ENABLE_GX` en lugar de `ENABLE_OPENGL`.

### 6. Sistema de archivos

**`soh/src/code/main.c`**:
- Incluye `<fat.h>` bajo `__wii__`.
- `Wii_MountSD()` llama `fatInitDefault()` una sola vez.
- `Wii_MountSD()` se invoca al inicio de `main()`.

**`libultraship/src/ship/Context.cpp`**:
- `GetAppBundlePath()` y `GetAppDirectoryPath()` devuelven `sd:/apps/soh` en Wii.

### 7. Memoria (MEM2)

**`soh/src/code/main.c`**:
- `u32 MALLOC_MEM2 = 1;` bajo `__wii__` (símbolo débil que libogc respeta).
- Fuerza el heap a MEM2 (64 MB) en lugar de MEM1 (24 MB compartidos con GPU).

### 8. Big-endian

**Automático** — el código de SoH detecta `__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__` (que devkitPPC define para Gekko/Broadway) y activa `IS_BIGENDIAN`. Las macros `BE16SWAP`/`LE16SWAP` en `endianness.h` hacen los swaps.

**Pendiente de manejar manualmente:**
- Texturas → GX (el layout `GX_TF_RGBA8` ya lo maneja `UploadTexture`).
- Matrices (`Mtx` N64 → formato GX).
- Guardado de partidas (decidir endianness de escritura).

### 9. Protección SDL en código compartido

**`libultraship/src/fast/Fast3dGui.cpp`**:
- Los `case FAST3D_SDL_OPENGL` y `FAST3D_SDL_METAL` envueltos en `#ifndef __wii__`.
- `DrawFloatingWindows()` devuelve temprano en Wii.
- `RefreshImGuiGamepads()` no hace nada en Wii.

---

##  Audio

El sistema de audio de SoH es **portable por diseño**. Jerarquía:

```

Audio (manager)
└── AudioPlayer (base abstracta)
├── SDLAudioPlayer      ← el que usaremos en Wii
├── CoreAudioAudioPlayer (macOS)
├── WasapiAudioPlayer    (Windows)
└── NullAudioPlayer      (fallback)

```

**En Wii se usa `SDLAudioPlayer`** porque:
- SDL2-wii (que viene con devkitPro) incluye un módulo de audio funcional.
- `SDLAudioPlayer` usa solo API estándar de SDL2: `SDL_OpenAudioDevice`, `SDL_QueueAudio`, `SDL_GetQueuedAudioSize`. **Ninguna requiere cambios para Wii.**
- El fallback automático a `NullAudioPlayer` en `Audio.cpp` garantiza que el juego arranque incluso si SDL audio falla.

**Configuración por defecto:**
- Sample rate: 44100 Hz (SDL2-wii resamplea a 48 kHz nativo si es necesario)
- Canales: 2 (estéreo) o 6 (5.1 matrix)
- Formato: `AUDIO_S16SYS` — en Wii esto significa **big-endian**, SDL lo maneja correctamente.

**Alternativa futura (si SDL audio en Wii da problemas):**
- Crear `ASNDAudioPlayer` usando `ASND_Init`, `ASND_SetVoice`, `ASND_AddVoice` de libogc.
- Menor latencia, control total del hardware. Más código y gestión de IRQ.

**Referencia (Xash3D-Wii):** el port de Xash3D usa SDL2 para todo su audio.

---

##  Shaders GX

En GX **no hay shaders programables**. La iluminación, texturizado y mezcla se hacen
con **TEV stages** (hasta 16), que son registros de hardware configurados en runtime.

### Traducción OpenGL → GX

| OpenGL | GX |
|--------|-----|
| `aVtxPos` (4 floats) | `GX_Position3f32` (x, y, z) |
| `aTexCoord0/1` (2 floats) | `GX_TexCoord2f32` |
| `aGrayscaleColor`, `aInput*` (3-4 floats) | `GX_Color4u8` |
| `aFog` (4 floats) | Ignorado por ahora |
| Vertex + Fragment shader | Combinación de TEV stages |
| `glVertexAttribPointer` con offsets | Cálculo manual de offsets |

### Estructura del `ShaderProgram` en GX

```cpp
struct ShaderProgram {
    uint8_t numInputs;
    uint8_t numFloats;              // Total de floats por vértice
    uint8_t numAttribs;
    bool usedTextures[SHADER_MAX_TEXTURES];

    // Offsets en floats dentro de cada vértice
    size_t posOffset;               // aVtxPos: x,y,z,w
    size_t tex0Offset;              // aTexCoord0: u,v
    size_t tex1Offset;              // aTexCoord1: u,v
    size_t colorOffset;             // aGrayscaleColor o aInput1
    size_t fogOffset;               // aFog: rgba

    bool hasTexture;
    bool hasAlpha;
    bool twoCycle;
    uint8_t numTevStages;
};
```

Layout del VBO (floats por vértice)

1. aVtxPos: 4 floats (x, y, z, w) — siempre
2. Para cada textura i con usedTextures[i]:
   · aTexCoord{i}: 2 floats (u, v)
   · Por cada clamp[i][j]: aTexClamp{S,T}{i}: 1 float
3. aFog: 4 floats (si opt_fog)
4. aGrayscaleColor: 4 floats (si opt_grayscale)
5. Por cada input n: aInput{n+1}: 4 floats (si opt_alpha) o 3 floats

No es un layout fijo de 9 floats. Cada shader tiene su propio numFloats
calculado por CreateAndLoadNewShader.

TEV: pendientes

· TEV 2-cycle (opt_2cyc): para shaders con 2 pasadas.
· Alpha blending más complejo (opt_alpha).
· Máscaras (used_masks): texturas adicionales con operaciones especiales.
· Blend de 2 texturas (used_blend).
· Clamp manual de coordenadas (clamp[i][j]).
· Fog (opt_fog): interpolación de color por profundidad.

Cada uno se mapea a combinaciones de GX_SetTevColorIn, GX_SetTevAlphaIn,
GX_SetTevColorOp, GX_SetTevAlphaOp. Ver libogc/ogc/gx.h para constantes.

---

 Compilación en Termux (validación ARM)

Sirve para validar que el código compila en ARM64 antes de saltar a devkitPPC.

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

# Build
cmake --build build-cmake -j$(nproc)
```

Resultado: build-cmake/soh/soh (~38 MB, ELF ARM64).

---

 Compilación para Wii (devkitPro)

Requiere PC con devkitPro. En WSL2, instalar Ubuntu y devkitPro:

```bash
# En WSL2 (Ubuntu)
sudo ln -s /proc/self/mounts /etc/mtab
wget https://apt.devkitpro.org/install-devkitpro-pacman
chmod +x ./install-devkitpro-pacman
sudo ./install-devkitpro-pacman
sudo dkp-pacman -S wii-dev
source /etc/profile.d/devkit-env.sh
```

Luego clonar y compilar:

```bash
git clone https://github.com/Gerardo-Hub17/Shipwright-4-Riivolution-Wii.git
cd Shipwright-4-Riivolution-Wii
git checkout wii-cmake-support

export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=$DEVKITPRO/devkitPPC

bash scripts/wii/build.sh
# o manualmente:
cmake -S . -B build-wii -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Wii.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-wii
```

Optimización crítica en WSL2: clona el repo en ~/ (ext4 nativo), no en /mnt/c/.... Compilar desde discos de Windows es 10-50× más lento.

---

 Referencias del backend GX (Xash3D-FWGS ref-gx)

Repo: https://github.com/Gerardo-Hub17/xash3d-fwgs/tree/ref-gx

Archivo Responsabilidad Uso en SoH
ref/gx/gx_local.h Structs, defines, tipos Inspiración para TextureInfoGX
engine/platform/ogc/vid_ogc.c:89-139 Init de VI + GX + framebuffer GfxRenderingAPIGX::Init()
ref/gx/gx_context.c:36-48 Vertex format + TEV base GfxRenderingAPIGX::StartFrame()
ref/gx/gx_image.c:111-127 GX_InitTexObj + GX_InitTexObjLOD GfxRenderingAPIGX::UploadTexture()
ref/gx/gx_image.c:505-565 Upload completo con DCFlushRange GfxRenderingAPIGX::UploadTexture()
ref/gx/gx_draw.c:42-56 GX_Begin/GX_End con quads GfxRenderingAPIGX::DrawTriangles()
ref/gx/gx_alias.c:708-898 Setup de TEV stages por shader GfxRenderingAPIGX::LoadShader()
ref/gx/gx_backend.c:180-186 Ejemplo de TEV con MODULATE/PASSCLR CreateAndLoadNewShader()
engine/platform/ogc/in_ogc.c:207-260 Input WPAD/PAD con mapeo a teclas GfxWindowBackendWii::HandleEvents()

---

 Pendientes y roadmap

Cuando arranque el primer build en devkitPPC

1. Resolver errores reales de compilación — van a salir varios.
2. Verificar std::filesystem — si falla, reescribir con opendir/readdir de libfat.
3. Ajustar el layout del VBO en DrawTriangles si algún shader no coincide.
4. Depurar TEV stages con contenido real en pantalla.

Bloqueantes actuales

· GetPixelDepth — sin efecto de escudo/daño funcionando.
· Framebuffers dinámicos — necesarios para render-to-texture (efectos de agua, telescopio).
· TEV 2-cycle y alpha blending — algunos efectos del juego podrían fallar.
· Matrices N64 → GX — actualmente pasa matrices 4×4 directas, puede que no sea 1:1.

Mejoras futuras

· Audio nativo ASND — si SDL2-wii da problemas, reescribir con libogc.
· Más endpoints de GfxWindowBackendWii — actualmente la mayoría son stubs.
· Adaptación de la GUI de SoH — la resolución de Wii (640×480) puede requerir ajustes.

---

 Lecciones aprendidas

Termux (ARM64)

· Termux define __ANDROID__ (bionic libc), pero no tiene las funciones JNI de SDL2. Guardar con #if defined(__ANDROID__) && !defined(TERMUX).
· __THROW de glibc no existe en bionic. Se resuelve con -D__THROW= en los flags.
· __assert declarada por el proyecto entra en conflicto con la de bionic. Eliminar del código.
· libzip busca zstd::libzstd_static, que no existe en Termux. Usar PATCH_COMMAND.
· Mesa en Termux provee libGL.so (GLVND). Enlazar con OpenGL::GL.

Wii

· No hay window manager. Fast3dWindow asume SDL2; hubo que crear GfxWindowBackendWii.
· No hay std::thread real en libogc. spdlog necesita SPDLOG_NO_TLS y SPDLOG_NO_THREAD_ID.
· La memoria es limitada (24 MB MEM1 + 64 MB MEM2). MALLOC_MEM2 = 1 es crítico.
· Los scancodes Wii no colisionan con PS/2 si empiezan en 1000. Así se evita pisar valores existentes.
· fatInitDefault() debe llamarse antes de cualquier fopen. En main() al arrancar.
· GX_TF_RGBA8 no es RGBA entrelazado. Es tiles 4×4 con dos planos (RA y GB). Ver UploadTexture.

Protocolo de edición

Nunca commitear sin compilar. El flujo correcto es:

1. Editar con nano (o sed/Python si es cambio puntual).
2. Guardar.
3. cmake --build build-cmake -j$(nproc) 2>&1 | tail -15.
4. Si compila: git add + git commit.
5. Si falla: arreglar antes de commitear.

Los CMakeLists con if/else/endif son especialmente frágiles a ediciones manuales. Usar grep y cat -n para verificar la estructura antes de guardar.
