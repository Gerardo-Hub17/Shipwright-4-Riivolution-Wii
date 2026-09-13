#ifdef ENABLE_GX

#include "fast/backends/gfx_gx.h"

#include <cstdio>
#include <cstring>

namespace Fast {

// ==================== Identificación ====================
const char* GfxRenderingAPIGX::GetName() {
    return "GX";
}

int GfxRenderingAPIGX::GetMaxTextureSize() {
    // GX soporta hasta 1024x1024 en texturas
    return 1024;
}

GfxClipParameters GfxRenderingAPIGX::GetClipParameters() {
    // GX: Z va de 0 a 1, Y está invertido respecto a OpenGL
    return { true, true };
}

// ==================== Shaders ====================
void GfxRenderingAPIGX::UnloadShader(ShaderProgram* oldPrg) {
    // TODO: liberar recursos GX del shader
    (void)oldPrg;
}

void GfxRenderingAPIGX::LoadShader(ShaderProgram* newPrg) {
    // TODO: configurar TEV stages con el nuevo shader
    (void)newPrg;
}

ShaderProgram* GfxRenderingAPIGX::CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) {
    // TODO: crear ShaderProgram y configurar TEV desde los shaderId
    (void)shaderId0;
    (void)shaderId1;
    return nullptr;
}

ShaderProgram* GfxRenderingAPIGX::LookupShader(uint64_t shaderId0, uint64_t shaderId1) {
    (void)shaderId0;
    (void)shaderId1;
    return nullptr;
}

void GfxRenderingAPIGX::ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    if (prg) {
        *numInputs = prg->numInputs;
        usedTextures[0] = prg->usedTextures[0];
        usedTextures[1] = prg->usedTextures[1];
    } else {
        *numInputs = 0;
        usedTextures[0] = false;
        usedTextures[1] = false;
    }
}

void GfxRenderingAPIGX::ClearShaderCache() {
    // TODO: limpiar cache de shaders
}

// ==================== Texturas ====================
uint32_t GfxRenderingAPIGX::NewTexture() {
    TextureInfoGX tex;
    mTextures.push_back(tex);
    return static_cast<uint32_t>(mTextures.size() - 1);
}

void GfxRenderingAPIGX::SelectTexture(int tile, uint32_t textureId) {
    // TODO: GX_LoadTexObj con el tile correspondiente
    (void)tile;
    (void)textureId;
}

void GfxRenderingAPIGX::UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) {
    // TODO: convertir RGBA32 a RGB5A3/RGBA8 y subir a GX
    (void)rgba32Buf;
    (void)width;
    (void)height;
}

void GfxRenderingAPIGX::SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) {
    (void)sampler;
    (void)linear_filter;
    (void)cms;
    (void)cmt;
}

void GfxRenderingAPIGX::DeleteTexture(uint32_t texId) {
    if (texId < mTextures.size()) {
        mTextures[texId] = TextureInfoGX{};
    }
}

void GfxRenderingAPIGX::SetTextureFilter(FilteringMode mode) {
    mCurrentFilterMode = mode;
}

FilteringMode GfxRenderingAPIGX::GetTextureFilter() {
    return mCurrentFilterMode;
}

ImTextureID GfxRenderingAPIGX::GetTextureById(int id) {
    (void)id;
    return (ImTextureID)0;
}

// ==================== Estado del pipeline ====================
void GfxRenderingAPIGX::SetDepthTestAndMask(bool depth_test, bool z_upd) {
    mCurrentDepthTest = depth_test ? 1 : 0;
    mCurrentDepthMask = z_upd ? 1 : 0;
    mPrimDepthDirty = true;
}

void GfxRenderingAPIGX::SetCurrentPrimDepth(float depth) {
    mCurrentPrimDepth = depth;
}

void GfxRenderingAPIGX::SetZmodeDecal(bool decal) {
    mCurrentZmodeDecal = decal ? 1 : 0;
    mPrimDepthDirty = true;
}

void GfxRenderingAPIGX::SetViewport(int x, int y, int width, int height) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void GfxRenderingAPIGX::SetScissor(int x, int y, int width, int height) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void GfxRenderingAPIGX::SetUseAlpha(bool useAlpha) {
    (void)useAlpha;
}

void GfxRenderingAPIGX::SetSrgbMode() {
    mSrgbMode = true;
}

// ==================== Dibujo ====================
void GfxRenderingAPIGX::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    // TODO: enviar vértices a GX (GX_Begin/GX_End o GX_CallDispList)
    (void)buf_vbo;
    (void)buf_vbo_len;
    (void)buf_vbo_num_tris;
}

// ==================== Ciclo de vida ====================
void GfxRenderingAPIGX::Init() {
    if (mInitialized) return;
    // TODO: GX_Init, configurar FIFO, TEV, etc.
    mInitialized = true;
}

void GfxRenderingAPIGX::OnResize() {
    // TODO: reconfigurar viewport al nuevo tamaño
}

void GfxRenderingAPIGX::StartFrame() {
    // TODO: GX_SetZMode, GX_SetBlendMode, GX_ClearVtxDesc, etc.
}

void GfxRenderingAPIGX::EndFrame() {
    // TODO: GX_DrawDone, GX_Flush
}

void GfxRenderingAPIGX::FinishRender() {
    // TODO
}

// ==================== Framebuffers ====================
int GfxRenderingAPIGX::CreateFramebuffer() {
    FramebufferGX fb;
    mFrameBuffers.push_back(fb);
    return static_cast<int>(mFrameBuffers.size() - 1);
}

void GfxRenderingAPIGX::UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                                     bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                                     bool can_extract_depth) {
    (void)opengl_invertY;
    (void)render_target;
    (void)can_extract_depth;
    if (fb_id < 0 || fb_id >= (int)mFrameBuffers.size()) return;
    auto& fb = mFrameBuffers[fb_id];
    fb.width = width;
    fb.height = height;
    fb.msaa_level = msaa_level;
    fb.has_depth_buffer = has_depth_buffer;
}

void GfxRenderingAPIGX::StartDrawToFramebuffer(int fbId, float noiseScale) {
    (void)noiseScale;
    if (fbId < 0 || fbId >= (int)mFrameBuffers.size()) return;
    mCurrentFrameBuffer = static_cast<size_t>(fbId);
    // TODO: GX_SetDispCopySrc/Dst
}

void GfxRenderingAPIGX::CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1,
                                        int dstX0, int dstY0, int dstX1, int dstY1) {
    (void)fbDstId; (void)fbSrcId;
    (void)srcX0; (void)srcY0; (void)srcX1; (void)srcY1;
    (void)dstX0; (void)dstY0; (void)dstX1; (void)dstY1;
}

void GfxRenderingAPIGX::ClearFramebuffer(bool color, bool depth) {
    (void)color;
    (void)depth;
    // TODO: GX_SetDispCopyYScale, etc.
}

void GfxRenderingAPIGX::ClearDepthRegion(int x, int y, int w, int h) {
    (void)x; (void)y; (void)w; (void)h;
    ClearFramebuffer(false, true);
}

void GfxRenderingAPIGX::ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) {
    (void)fbId; (void)width; (void)height; (void)rgba16Buf;
    // TODO: GX_SetTexCopySrc/Dst + GX_CopyTex
}

void GfxRenderingAPIGX::ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) {
    (void)fbIdTarger; (void)fbIdSrc;
    // GX no tiene MSAA nativo como GL
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIGX::GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    (void)fb_id;
    (void)coordinates;
    return {};
}

void* GfxRenderingAPIGX::GetFramebufferTextureId(int fbId) {
    (void)fbId;
    return nullptr;
}

void GfxRenderingAPIGX::SelectTextureFb(int fbId) {
    (void)fbId;
}

} // namespace Fast
#endif // ENABLE_GX
