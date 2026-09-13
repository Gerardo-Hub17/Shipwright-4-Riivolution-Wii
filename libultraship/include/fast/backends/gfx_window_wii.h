#pragma once

#ifdef ENABLE_GX

#include "gfx_window_manager_api.h"

namespace Fast {

/**
 * @brief Ventana / input backend para Nintendo Wii.
 *
 * En Wii no hay ventanas. Este backend:
 *  - inicializa WPAD (Wiimote) y PAD (GameCube),
 *  - escanea los mandos cada frame y emite callbacks de teclado,
 *  - devuelve el timing con ticks_to_millisecs(gettime()),
 *  - y deja el resto (mouse, resize, fullscreen) como stubs.
 *
 * El swap de framebuffer lo hace GfxRenderingAPIGX::EndFrame().
 */
class GfxWindowBackendWii final : public GfxWindowBackend {
  public:
    ~GfxWindowBackendWii() override = default;

    void Init(const char* gameName, const char* apiName, bool startFullScreen,
              uint32_t width, uint32_t height, int32_t posX, int32_t posY) override;
    void Close() override;
    void SetKeyboardCallbacks(bool (*onKeyDown)(int scancode),
                              bool (*onKeyUp)(int scancode),
                              void (*onAllKeysUp)()) override;
    void SetMouseCallbacks(bool (*onMouseButtonDown)(int btn),
                           bool (*onMouseButtonUp)(int btn)) override;
    void SetFullscreenChangedCallback(void (*onFullscreenChanged)(bool isNowFullscreen)) override;
    void SetFullscreen(bool fullscreen) override;
    void GetActiveWindowRefreshRate(uint32_t* refreshRate) override;
    void SetCursorVisibility(bool visibility) override;
    void SetMousePos(int32_t posX, int32_t posY) override;
    void GetMousePos(int32_t* x, int32_t* y) override;
    void GetMouseDelta(int32_t* x, int32_t* y) override;
    void GetMouseWheel(float* x, float* y) override;
    bool GetMouseState(uint32_t btn) override;
    void SetMouseCapture(bool capture) override;
    bool IsMouseCaptured() override;
    void GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) override;
    void SetDimensions(uint32_t width, uint32_t height, int32_t posX, int32_t posY) override;
    Ship::WindowRect GetPrimaryMonitorRect() override;
    void HandleEvents() override;
    bool IsFrameReady() override;
    void SwapBuffersBegin() override;
    void SwapBuffersEnd() override;
    double GetTime() override;
    int GetTargetFps() override;
    void SetTargetFps(int fps) override;
    void SetMaxFrameLatency(int latency) override;
    const char* GetKeyName(int scancode) override;
    bool CanDisableVsync() override;
    bool IsRunning() override;
    void Destroy() override;
    bool IsFullscreen() override;

  private:
    // Estado de input del frame anterior (para detectar flancos).
    uint32_t mPrevWpadButtons = 0;
    uint32_t mPrevPadButtons = 0;

    // Timestamp base en milisegundos (ticks_to_millisecs).
    uint64_t mStartTicks = 0;

    // Callback "all keys up" que no está en la clase base.
    void (*mOnAllKeysUp)() = nullptr;
};

} // namespace Fast

#endif // ENABLE_GX
