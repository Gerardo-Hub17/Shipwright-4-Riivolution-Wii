#pragma once

#ifdef ENABLE_GX

#include "gfx_rendering_api.h"
#include "../interpreter.h"

#include <map>
#include <utility>

#include <gccore.h>
#include <ogc/gx.h>
#include <ogc/gu.h>

namespace Fast {

// Tamaño máximo del pool de shaders en GX (TEV stages)
constexpr size_t GX_MAX_ATTRIBS = 16;

/**
 * @brief Shader equivalente en GX.
 *
 * En GX no hay shaders programables: la combinación de colores se hace con
 * TEV stages (hasta 16). Este struct guarda los offsets reales dentro del VBO
 * de SoH (que tiene layout dinámico según qué atributos active el shader),
 * y el shader ID original para poder reproducir el TEV correcto.
 */
struct ShaderProgram {
    uint8_t numInputs = 0;
    uint8_t numFloats = 0;              ///< Total de floats por vértice en el VBO.
    uint8_t numAttribs = 0;
    bool usedTextures[SHADER_MAX_TEXTURES] = {};

    // Offsets en FLOATS dentro de cada vértice (no bytes).
    // SIZE_MAX = atributo ausente.
    size_t posOffset = 0;               ///< aVtxPos (x, y, z, w)
    size_t tex0Offset = SIZE_MAX;       ///< aTexCoord0 (u, v)
    size_t tex1Offset = SIZE_MAX;       ///< aTexCoord1 (u, v)
    size_t colorOffset = SIZE_MAX;      ///< Primer aInput o aGrayscaleColor (RGBA)
    size_t fogOffset = SIZE_MAX;        ///< aFog (RGBA)

    // Configuración TEV calculada a partir del shader_id.
    bool hasTexture = false;
    bool hasAlpha = false;
    bool twoCycle = false;
    uint8_t numTevStages = 1;
};

struct FramebufferGX {
    uint32_t width = 0, height = 0;
    bool has_depth_buffer = false;
    uint32_t msaa_level = 1;
    bool invertY = false;
    void* colorBuffer = nullptr;
    void* depthBuffer = nullptr;
    uint32_t textureId = 0;
};

struct TextureInfoGX {
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t filtering = 0;
    void* data = nullptr;
    GXTexObj texObj = {};
};

class GfxRenderingAPIGX final : public GfxRenderingAPI {
  public:
    ~GfxRenderingAPIGX() override = default;

    // --- Identificación ---
    const char* GetName() override;
    int GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;

    // --- Shaders ---
    void UnloadShader(ShaderProgram* oldPrg) override;
    void LoadShader(ShaderProgram* newPrg) override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1) override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;
    void ClearShaderCache() override;

    // --- Texturas ---
    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t textureId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override;
    void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) override;
    void DeleteTexture(uint32_t texId) override;
    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override;
    ImTextureID GetTextureById(int id) override;

    // --- Estado del pipeline ---
    void SetDepthTestAndMask(bool depth_test, bool z_upd) override;
    void SetCurrentPrimDepth(float depth) override;
    void SetZmodeDecal(bool decal) override;
    void SetViewport(int x, int y, int width, int height) override;
    void SetScissor(int x, int y, int width, int height) override;
    void SetUseAlpha(bool useAlpha) override;
    void SetSrgbMode() override;

    // --- Dibujo ---
    void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) override;

    // --- Ciclo de vida ---
    void Init() override;
    void OnResize() override;
    void StartFrame() override;
    void EndFrame() override;
    void FinishRender() override;

    // --- Framebuffers ---
    int CreateFramebuffer() override;
    void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                     bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                     bool can_extract_depth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
                         int dstY0, int dstX1, int dstY1) override;
    void ClearFramebuffer(bool color, bool depth) override;
    void ClearDepthRegion(int x, int y, int w, int h) override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) override;
    void ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) override;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) override;
    void* GetFramebufferTextureId(int fbId) override;
    void SelectTextureFb(int fbId) override;

  private:
    // Aplica la configuración TEV según el shader activo.
    void ApplyTevForShader(const ShaderProgram* prg);

    std::vector<TextureInfoGX> mTextures;
    std::vector<FramebufferGX> mFrameBuffers;
    size_t mCurrentFrameBuffer = 0;
    FilteringMode mCurrentFilterMode = FILTER_THREE_POINT;
    bool mInitialized = false;

    // Pool de shaders (clave = par de IDs).
    std::map<std::pair<uint64_t, uint32_t>, ShaderProgram> mShaderProgramPool;
    ShaderProgram* mCurrentShaderProgram = nullptr;
    ShaderProgram* mLastLoadedShader = nullptr;
};

} // namespace Fast
#endif // ENABLE_GX
