#ifdef ENABLE_GX

#include "fast/backends/gfx_window_wii.h"

#include <cstdio>
#include <cstring>

#include <gccore.h>
#include <ogc/system.h>
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>

#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"

namespace Fast {

// ============================================================
//  Mapeo de botones Wii -> scancodes internos de SoH.
//
//  Usamos un rango >= 1000 para no colisionar con los scancodes
//  SDL (que son valores pequeños). Cuando SoH tenga su sistema
//  de controller configurado para Wii, habrá que registrar estos
//  valores ahí (ver KbScancode).
// ============================================================
// Alias de los scancodes Wii definidos en Ship::KbScancode.
// Ver libultraship/include/ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h
namespace WiiScan {
enum : int {
    A       = Ship::LUS_KB_WII_A,
    B       = Ship::LUS_KB_WII_B,
    X       = Ship::LUS_KB_WII_X,
    Y       = Ship::LUS_KB_WII_Y,
    START   = Ship::LUS_KB_WII_START,
    SELECT  = Ship::LUS_KB_WII_SELECT,
    HOME    = Ship::LUS_KB_WII_HOME,
    L       = Ship::LUS_KB_WII_L,
    R       = Ship::LUS_KB_WII_R,
    ZL      = Ship::LUS_KB_WII_ZL,
    ZR      = Ship::LUS_KB_WII_ZR,
    DPAD_UP    = Ship::LUS_KB_WII_DPAD_UP,
    DPAD_DOWN  = Ship::LUS_KB_WII_DPAD_DOWN,
    DPAD_LEFT  = Ship::LUS_KB_WII_DPAD_LEFT,
    DPAD_RIGHT = Ship::LUS_KB_WII_DPAD_RIGHT,
    STICK_UP    = Ship::LUS_KB_WII_STICK_UP,
    STICK_DOWN  = Ship::LUS_KB_WII_STICK_DOWN,
    STICK_LEFT  = Ship::LUS_KB_WII_STICK_LEFT,
    STICK_RIGHT = Ship::LUS_KB_WII_STICK_RIGHT,
};
} // namespace WiiScan

struct WiiButtonMap {
    uint32_t mask;
    int scancode;
};

// --- Wiimote solo (o con nunchuk) ---
static const WiiButtonMap kMapWiimote[] = {
    { WPAD_BUTTON_A,      WiiScan::A },
    { WPAD_BUTTON_B,      WiiScan::B },
    { WPAD_BUTTON_1,      WiiScan::X },
    { WPAD_BUTTON_2,      WiiScan::Y },
    { WPAD_BUTTON_MINUS,  WiiScan::SELECT },
    { WPAD_BUTTON_PLUS,   WiiScan::START },
    { WPAD_BUTTON_HOME,   WiiScan::HOME },
    { WPAD_BUTTON_UP,     WiiScan::DPAD_UP },
    { WPAD_BUTTON_DOWN,   WiiScan::DPAD_DOWN },
    { WPAD_BUTTON_LEFT,   WiiScan::DPAD_LEFT },
    { WPAD_BUTTON_RIGHT,  WiiScan::DPAD_RIGHT },
    { WPAD_NUNCHUK_BUTTON_C, WiiScan::L },
    { WPAD_NUNCHUK_BUTTON_Z, WiiScan::R },
};

// --- Classic Controller ---
static const WiiButtonMap kMapClassic[] = {
    { WPAD_CLASSIC_BUTTON_A,      WiiScan::A },
    { WPAD_CLASSIC_BUTTON_B,      WiiScan::B },
    { WPAD_CLASSIC_BUTTON_X,      WiiScan::X },
    { WPAD_CLASSIC_BUTTON_Y,      WiiScan::Y },
    { WPAD_CLASSIC_BUTTON_FULL_L, WiiScan::L },
    { WPAD_CLASSIC_BUTTON_FULL_R, WiiScan::R },
    { WPAD_CLASSIC_BUTTON_ZL,     WiiScan::ZL },
    { WPAD_CLASSIC_BUTTON_ZR,     WiiScan::ZR },
    { WPAD_CLASSIC_BUTTON_MINUS,  WiiScan::SELECT },
    { WPAD_CLASSIC_BUTTON_PLUS,   WiiScan::START },
    { WPAD_CLASSIC_BUTTON_HOME,   WiiScan::HOME },
    { WPAD_CLASSIC_BUTTON_UP,     WiiScan::DPAD_UP },
    { WPAD_CLASSIC_BUTTON_DOWN,   WiiScan::DPAD_DOWN },
    { WPAD_CLASSIC_BUTTON_LEFT,   WiiScan::DPAD_LEFT },
    { WPAD_CLASSIC_BUTTON_RIGHT,  WiiScan::DPAD_RIGHT },
};

// --- GameCube ---
static const WiiButtonMap kMapGameCube[] = {
    { PAD_BUTTON_A,     WiiScan::A },
    { PAD_BUTTON_B,     WiiScan::B },
    { PAD_BUTTON_X,     WiiScan::X },
    { PAD_BUTTON_Y,     WiiScan::Y },
    { PAD_TRIGGER_L,    WiiScan::L },
    { PAD_TRIGGER_R,    WiiScan::R },
    { PAD_TRIGGER_Z,    WiiScan::ZR },
    { PAD_BUTTON_START, WiiScan::START },
    { PAD_BUTTON_UP,    WiiScan::DPAD_UP },
    { PAD_BUTTON_DOWN,  WiiScan::DPAD_DOWN },
    { PAD_BUTTON_LEFT,  WiiScan::DPAD_LEFT },
    { PAD_BUTTON_RIGHT, WiiScan::DPAD_RIGHT },
};

// Emite callbacks según los botones cambiados este frame.
static void EmitButtons(const WiiButtonMap* map, size_t count,
                        uint32_t changed, uint32_t held,
                        bool (*onKeyDown)(int), bool (*onKeyUp)(int)) {
    for (size_t i = 0; i < count; ++i) {
        if (changed & map[i].mask) {
            bool pressed = (held & map[i].mask) != 0;
            if (pressed && onKeyDown) {
                onKeyDown(map[i].scancode);
            } else if (!pressed && onKeyUp) {
                onKeyUp(map[i].scancode);
            }
        }
    }
}

// ============================================================
//  Init / Close
// ============================================================
void GfxWindowBackendWii::Init(const char* gameName, const char* apiName,
                               bool startFullScreen, uint32_t width, uint32_t height,
                               int32_t posX, int32_t posY) {
    (void)gameName; (void)apiName; (void)startFullScreen;
    (void)width; (void)height; (void)posX; (void)posY;

    WPAD_Init();
    PAD_Init();

    mStartTicks = gettime();
    mIsRunning  = true;
    mFullScreen = true;
}

void GfxWindowBackendWii::Close() {
    mIsRunning = false;
}

void GfxWindowBackendWii::Destroy() {
    mIsRunning = false;
}

// ============================================================
//  Callbacks
// ============================================================
void GfxWindowBackendWii::SetKeyboardCallbacks(bool (*onKeyDown)(int),
                                                bool (*onKeyUp)(int),
                                                void (*onAllKeysUp)()) {
    mOnKeyDown   = onKeyDown;
    mOnKeyUp     = onKeyUp;
    mOnAllKeysUp = onAllKeysUp;
}

void GfxWindowBackendWii::SetMouseCallbacks(bool (*onMouseButtonDown)(int),
                                             bool (*onMouseButtonUp)(int)) {
    mOnMouseButtonDown = onMouseButtonDown;
    mOnMouseButtonUp   = onMouseButtonUp;
}

void GfxWindowBackendWii::SetFullscreenChangedCallback(void (*onFullscreenChanged)(bool)) {
    mOnFullscreenChanged = onFullscreenChanged;
}

// ============================================================
//  Loop de eventos
// ============================================================
void GfxWindowBackendWii::HandleEvents() {
    // WPAD_ScanPads debe llamarse cada frame (SDL no lo hace en Wii).
    WPAD_ScanPads();
    // PAD_ScanPads normalmente lo llama SDL internamente, pero como
    // aquí no hay SDL, lo hacemos nosotros.
    PAD_ScanPads();

    uint32_t wpadHeld = WPAD_ButtonsHeld(WPAD_CHAN_0);
    uint32_t padHeld  = (uint32_t)PAD_ButtonsHeld(PAD_CHAN0);

    uint32_t wpadChanged = wpadHeld ^ mPrevWpadButtons;
    uint32_t padChanged  = padHeld  ^ mPrevPadButtons;

    if (mOnKeyDown || mOnKeyUp) {
        // Detectar Classic Controller vs Wiimote
        uint32_t expType = WPAD_EXP_NONE;
        if (WPAD_Probe(WPAD_CHAN_0, &expType) == WPAD_ERR_NONE
            && expType == WPAD_EXP_CLASSIC) {
            EmitButtons(kMapClassic, sizeof(kMapClassic) / sizeof(kMapClassic[0]),
                        wpadChanged, wpadHeld, mOnKeyDown, mOnKeyUp);
        } else {
            EmitButtons(kMapWiimote, sizeof(kMapWiimote) / sizeof(kMapWiimote[0]),
                        wpadChanged, wpadHeld, mOnKeyDown, mOnKeyUp);
        }

        EmitButtons(kMapGameCube, sizeof(kMapGameCube) / sizeof(kMapGameCube[0]),
                    padChanged, padHeld, mOnKeyDown, mOnKeyUp);
    }

    mPrevWpadButtons = wpadHeld;
    mPrevPadButtons  = padHeld;

    // El botón HOME apaga la consola por defecto. Si queremos interceptarlo
    // if (wpadHeld & WPAD_BUTTON_HOME) { ... }
    (void)mOnAllKeysUp; 
}

// ============================================================
//  Framebuffer / timing
// ============================================================
bool GfxWindowBackendWii::IsFrameReady() {
    // En Wii con vsync a 60Hz
    return true;
}

void GfxWindowBackendWii::SwapBuffersBegin() 

void GfxWindowBackendWii::SwapBuffersEnd() {
    // Idem.
}

double GfxWindowBackendWii::GetTime() {
    uint64_t elapsed = gettime() - mStartTicks;
    return (double)ticks_to_millisecs(elapsed) / 1000.0;
}

int  GfxWindowBackendWii::GetTargetFps() { return (int)mTargetFps; }
void GfxWindowBackendWii::SetTargetFps(int fps) { mTargetFps = (uint32_t)fps; }
void GfxWindowBackendWii::SetMaxFrameLatency(int latency) { (void)latency; }

void GfxWindowBackendWii::GetActiveWindowRefreshRate(uint32_t* refreshRate) {
    if (refreshRate) *refreshRate = 60;
}

// ============================================================
//  Dimensiones (constantes en Wii)
// ============================================================
void GfxWindowBackendWii::GetDimensions(uint32_t* width, uint32_t* height,
                                         int32_t* posX, int32_t* posY) {
    if (width)  *width  = 640;
    if (height) *height = 480;
    if (posX)   *posX   = 0;
    if (posY)   *posY   = 0;
}

void GfxWindowBackendWii::SetDimensions(uint32_t, uint32_t, int32_t, int32_t) {
    // No-op: la Wii siempre corre a 640x480 (NTSC) o 720x480 (PAL).
}

Ship::WindowRect GfxWindowBackendWii::GetPrimaryMonitorRect() {
    Ship::WindowRect rect{};
    return rect;
}

// ============================================================
//  Fullscreen / ventana
// ============================================================
void GfxWindowBackendWii::SetFullscreen(bool fullscreen) {
    (void)fullscreen;
    mFullScreen = true;
}

bool GfxWindowBackendWii::IsFullscreen() { return true; }
bool GfxWindowBackendWii::IsRunning()    { return mIsRunning; }
bool GfxWindowBackendWii::CanDisableVsync() { return false; }

void GfxWindowBackendWii::SetCursorVisibility(bool) { /* no hay cursor */ }

// ============================================================
//  Mouse (stubs, no existe en Wii)
// ============================================================
void GfxWindowBackendWii::SetMousePos(int32_t, int32_t) {}
void GfxWindowBackendWii::GetMousePos(int32_t* x, int32_t* y) {
    if (x) *x = 0;
    if (y) *y = 0;
}
void GfxWindowBackendWii::GetMouseDelta(int32_t* x, int32_t* y) {
    if (x) *x = 0;
    if (y) *y = 0;
}
void GfxWindowBackendWii::GetMouseWheel(float* x, float* y) {
    if (x) *x = 0.0f;
    if (y) *y = 0.0f;
}
bool GfxWindowBackendWii::GetMouseState(uint32_t) { return false; }
void GfxWindowBackendWii::SetMouseCapture(bool) {}
bool GfxWindowBackendWii::IsMouseCaptured() { return false; }

// ============================================================
//  Nombres de teclas
// ============================================================
const char* GfxWindowBackendWii::GetKeyName(int scancode) {
    switch (scancode) {
        case WiiScan::A:           return "A";
        case WiiScan::B:           return "B";
        case WiiScan::X:           return "X";
        case WiiScan::Y:           return "Y";
        case WiiScan::START:       return "Start";
        case WiiScan::SELECT:      return "Select";
        case WiiScan::HOME:        return "Home";
        case WiiScan::L:           return "L";
        case WiiScan::R:           return "R";
        case WiiScan::ZL:          return "ZL";
        case WiiScan::ZR:          return "ZR";
        case WiiScan::DPAD_UP:     return "D-Pad Up";
        case WiiScan::DPAD_DOWN:   return "D-Pad Down";
        case WiiScan::DPAD_LEFT:   return "D-Pad Left";
        case WiiScan::DPAD_RIGHT:  return "D-Pad Right";
        case WiiScan::STICK_UP:    return "Stick Up";
        case WiiScan::STICK_DOWN:  return "Stick Down";
        case WiiScan::STICK_LEFT:  return "Stick Left";
        case WiiScan::STICK_RIGHT: return "Stick Right";
        default:                   return "Unknown";
    }
}

} // namespace Fast

#endif // ENABLE_GX
