#ifdef ENABLE_GX

#include "fast/backends/gfx_gx.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <malloc.h>

#include <gccore.h>
#include <ogc/gx.h>
#include <ogc/gu.h>
#include <ogc/video.h>
#include <ogc/system.h>

namespace Fast {

// ============================================================
//  Estado interno (static para no tocar el header)
// ============================================================
static void*  sFifoBuffer      = nullptr;
static constexpr u32 FIFO_SIZE = 256 * 1024;

static void*  sFramebuffer[2]  = { nullptr, nullptr };
static u32    sFramebufferIdx  = 0;
static GXRModeObj* sVideoMode  = nullptr;
static u16    sWidth           = 640;
static u16    sHeight          = 480;

// Convierte RGBA32 entrelazado a GX_TF_RGBA8 (tiles 4x4, dos planos)
// Layout GX_TF_RGBA8 por tile (64 bytes):
//   [0..31]:  RA plane — por fila: R0 A0 R1 A1 R2 A2 R3 A3
//   [32..63]: GB plane — por fila: G0 B0 G1 B1 G2 B2 G3 B3
static void ConvertRGBA32ToGX(const uint8_t* src, uint8_t* dst,
                              uint32_t width, uint32_t height) {
    const uint32_t tilesX = (width  + 3) / 4;
    const uint32_t tilesY = (height + 3) / 4;
    uint8_t* out = dst;

    for (uint32_t ty = 0; ty < tilesY; ++ty) {
        for (uint32_t tx = 0; tx < tilesX; ++tx) {
            uint8_t* raPlane = out;
            uint8_t* gbPlane = out + 32;

            for (uint32_t row = 0; row < 4; ++row) {
                for (uint32_t col = 0; col < 4; ++col) {
                    uint32_t px = tx * 4 + col;
                    uint32_t py = ty * 4 + row;
                    uint8_t r = 0, g = 0, b = 0, a = 0;
                    if (px < width && py < height) {
                        const uint8_t* p = src + (py * width + px) * 4;
                        r = p[0]; g = p[1]; b = p[2]; a = p[3];
                    }
                    uint32_t off = row * 8 + col * 2;
                    raPlane[off + 0] = r;
                    raPlane[off + 1] = a;
                    gbPlane[off + 0] = g;
                    gbPlane[off + 1] = b;
                }
            }
            out += 64;
        }
    }
}

// ============================================================
//  Identificación
// ============================================================
const char* GfxRenderingAPIGX::GetName() { return "GX"; }
int GfxRenderingAPIGX::GetMaxTextureSize() { return 1024; }

GfxClipParameters GfxRenderingAPIGX::GetClipParameters() {
    // GX: Z en [0,1]; Y ya viene invertido por el projection matrix de SoH
    return { true, false };
}

// ============================================================
//  Ciclo de vida
// ============================================================
void GfxRenderingAPIGX::Init() {
    if (mInitialized) return;

    VIDEO_Init();
    sVideoMode = VIDEO_GetPreferredMode(nullptr);
    sWidth  = sVideoMode->fbWidth;
    sHeight = sVideoMode->efbHeight;

    // Framebuffers
    sFramebuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(sVideoMode));
    sFramebuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(sVideoMode));
    sFramebufferIdx = 0;

    // FIFO de comandos GX
    sFifoBuffer = MEM_K0_TO_K1(SYS_AllocateFifo(FIFO_SIZE));

    VIDEO_Configure(sVideoMode);
    VIDEO_SetNextFramebuffer(sFramebuffer[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (sVideoMode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // GX
    GX_Init(sFifoBuffer, FIFO_SIZE);

    GXColor bg = { 0, 0, 0, 0 };
    GX_SetCopyClear(bg, 0x00FFFFFF);

    GX_SetViewport(0, 0, sVideoMode->fbWidth, sVideoMode->efbHeight, 0.0f, 1.0f);

    f32 yscale = GX_GetYScaleFactor(sVideoMode->efbHeight, sVideoMode->xfbHeight);
    u32 xfbH   = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0, 0, sVideoMode->fbWidth, sVideoMode->efbHeight);
    GX_SetDispCopySrc(0, 0, sVideoMode->fbWidth, sVideoMode->efbHeight);
    GX_SetDispCopyDst(sVideoMode->fbWidth, xfbH);
    GX_SetCopyFilter(sVideoMode->aa, sVideoMode->sample_pattern, GX_TRUE, sVideoMode->vfilter);
    GX_SetFieldMode(sVideoMode->field_rendering,
                    (sVideoMode->viHeight == 2 * sVideoMode->xfbHeight) ? GX_ENABLE : GX_DISABLE);
    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetDispCopyGamma(GX_GM_1_0);

    // Configuración base del vertex format
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ, GX_F32,   0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST,  GX_F32,   0);

    GX_SetNumChans(1);
    GX_SetNumTexGens(1);
    GX_SetNumTevStages(1);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

    // Alpha blending base
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);

    // Cull y Z
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetZCompLoc(GX_FALSE);

    mInitialized = true;
}

void GfxRenderingAPIGX::OnResize() {
    if (!sVideoMode) return;
    GX_SetViewport(0, 0, sVideoMode->fbWidth, sVideoMode->efbHeight, 0.0f, 1.0f);
    GX_SetScissor(0, 0, sVideoMode->fbWidth, sVideoMode->efbHeight);
}

void GfxRenderingAPIGX::StartFrame() {
    // Configuración por frame
    GX_SetZMode(GX_TRUE, GX_LEQUAL, mCurrentDepthMask ? GX_TRUE : GX_FALSE);
    GX_SetCullMode(mCurrentZmodeDecal ? GX_CULL_NONE : GX_CULL_BACK);
}

void GfxRenderingAPIGX::EndFrame() {
    GX_DrawDone();

    // Flip de framebuffer
    sFramebufferIdx ^= 1;
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(sFramebuffer[sFramebufferIdx], GX_TRUE);
    GX_Flush();

    VIDEO_SetNextFramebuffer(sFramebuffer[sFramebufferIdx]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

void GfxRenderingAPIGX::FinishRender() {
    GX_DrawDone();
}

// ============================================================
//  Texturas
// ============================================================
uint32_t GfxRenderingAPIGX::NewTexture() {
    TextureInfoGX tex;
    mTextures.push_back(tex);
    return static_cast<uint32_t>(mTextures.size() - 1);
}

void GfxRenderingAPIGX::DeleteTexture(uint32_t texId) {
    if (texId < mTextures.size()) {
        if (mTextures[texId].data) {
            free(mTextures[texId].data);
        }
        mTextures[texId] = TextureInfoGX{};
    }
}

void GfxRenderingAPIGX::UploadTexture(const uint8_t* rgba32Buf,
                                      uint32_t width, uint32_t height) {
    if (mTextures.empty()) return;
    TextureInfoGX& tex = mTextures.back();

    // Ajusta a múltiplos de 4 (requisito de tiles GX)
    uint32_t w = (width  + 3) & ~3u;
    uint32_t h = (height + 3) & ~3u;

    // Tamaño en bytes del buffer GX_TF_RGBA8
    size_t gxSize = (size_t)w * h * 4;
    if (tex.data) free(tex.data);
    tex.data = memalign(32, gxSize);
    if (!tex.data) return;

    ConvertRGBA32ToGX(rgba32Buf, (uint8_t*)tex.data, width, height);

    // Flush de caché (obligatorio: GX lee directo de RAM)
    DCFlushRange(tex.data, (u32)gxSize);
    GX_InvalidateTexAll();

    tex.width  = (uint16_t)width;
    tex.height = (uint16_t)height;

    GX_InitTexObj(&tex.texObj, tex.data, (u16)width, (u16)height,
                  GX_TF_RGBA8, GX_REPEAT, GX_REPEAT, GX_FALSE);
    GX_InitTexObjFilterMode(&tex.texObj, GX_NEAR, GX_NEAR);
}

void GfxRenderingAPIGX::SelectTexture(int tile, uint32_t textureId) {
    if (textureId >= mTextures.size()) return;
    TextureInfoGX& tex = mTextures[textureId];
    if (!tex.data) return;

    GX_LoadTexObj(&tex.texObj, tile == 0 ? GX_TEXMAP0 : GX_TEXMAP1);
}

void GfxRenderingAPIGX::SetSamplerParameters(int sampler, bool linear_filter,
                                              uint32_t cms, uint32_t cmt) {
    (void)sampler; (void)cms; (void)cmt;
    if (mTextures.empty()) return;
    TextureInfoGX& tex = mTextures.back();
    if (!tex.data) return;
    u8 filt = linear_filter ? GX_LINEAR : GX_NEAR;
    GX_InitTexObjFilterMode(&tex.texObj, filt, filt);
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

// ============================================================
//  Estado del pipeline
// ============================================================
void GfxRenderingAPIGX::SetDepthTestAndMask(bool depth_test, bool z_upd) {
    mCurrentDepthTest = depth_test ? 1 : 0;
    mCurrentDepthMask = z_upd ? 1 : 0;
    GX_SetZMode(depth_test ? GX_TRUE : GX_FALSE, GX_LEQUAL, z_upd ? GX_TRUE : GX_FALSE);
}

void GfxRenderingAPIGX::SetCurrentPrimDepth(float depth) {
    mCurrentPrimDepth = depth;
}

void GfxRenderingAPIGX::SetZmodeDecal(bool decal) {
    mCurrentZmodeDecal = decal ? 1 : 0;
}

void GfxRenderingAPIGX::SetViewport(int x, int y, int width, int height) {
    GX_SetViewport((f32)x, (f32)y, (f32)width, (f32)height, 0.0f, 1.0f);
}

void GfxRenderingAPIGX::SetScissor(int x, int y, int width, int height) {
    GX_SetScissor((u32)x, (u32)y, (u32)width, (u32)height);
}

void GfxRenderingAPIGX::SetUseAlpha(bool useAlpha) {
    (void)useAlpha;
}

void GfxRenderingAPIGX::SetSrgbMode() { mSrgbMode = true; }

// ============================================================
//  Dibujo
// ============================================================
void GfxRenderingAPIGX::DrawTriangles(float buf_vbo[], size_t buf_vbo_len,
                                       size_t buf_vbo_num_tris) {
    if (buf_vbo_num_tris == 0 || buf_vbo_len == 0) return;

    // SoH empaqueta cada vértice como: x, y, z, u, v, r, g, b, a  (9 floats)
    // Si el layout difiere, ajustar aquí (ver gfx_opengl.cpp para el contrato real).
    constexpr size_t FLOATS_PER_VERTEX = 9;

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, (u16)(buf_vbo_num_tris * 3));

    size_t numVerts = buf_vbo_len / FLOATS_PER_VERTEX;
    for (size_t i = 0; i < numVerts; ++i) {
        const float* v = &buf_vbo[i * FLOATS_PER_VERTEX];
        GX_Position3f32(v[0], v[1], v[2]);
        GX_TexCoord2f32(v[3], v[4]);
        GX_Color4u8((u8)(v[5] * 255.0f), (u8)(v[6] * 255.0f),
                    (u8)(v[7] * 255.0f), (u8)(v[8] * 255.0f));
    }

    GX_End();
}

// ============================================================
//  Shaders (TEV)
// ============================================================
void GfxRenderingAPIGX::UnloadShader(ShaderProgram* oldPrg) { (void)oldPrg; }

void GfxRenderingAPIGX::LoadShader(ShaderProgram* newPrg) {
    if (!newPrg) return;

    // TODO: mapear los shader IDs a una combinación de TEV stages.
    // Por ahora, configuramos un estado razonable por defecto.
    if (newPrg->usedTextures[0]) {
        GX_SetNumTevStages(1);
        GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    } else {
        GX_SetNumTevStages(1);
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    }
}

ShaderProgram* GfxRenderingAPIGX::CreateAndLoadNewShader(uint64_t shaderId0,
                                                          uint64_t shaderId1) {
    // TODO: crear ShaderProgram y mapear shader IDs a TEV
    (void)shaderId0; (void)shaderId1;
    return nullptr;
}

ShaderProgram* GfxRenderingAPIGX::LookupShader(uint64_t shaderId0,
                                                uint64_t shaderId1) {
    (void)shaderId0; (void)shaderId1;
    return nullptr;
}

void GfxRenderingAPIGX::ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs,
                                       bool usedTextures[2]) {
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

void GfxRenderingAPIGX::ClearShaderCache() { /* TODO */ }

// ============================================================
//  Framebuffers (stubs por ahora, se implementan cuando haga falta)
// ============================================================
int GfxRenderingAPIGX::CreateFramebuffer() {
    FramebufferGX fb;
    mFrameBuffers.push_back(fb);
    return (int)(mFrameBuffers.size() - 1);
}

void GfxRenderingAPIGX::UpdateFramebufferParameters(
    int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
    bool opengl_invertY, bool render_target, bool has_depth_buffer,
    bool can_extract_depth) {
    (void)opengl_invertY; (void)render_target; (void)can_extract_depth;
    if (fb_id < 0 || fb_id >= (int)mFrameBuffers.size()) return;
    auto& fb = mFrameBuffers[fb_id];
    fb.width = width; fb.height = height;
    fb.msaa_level = msaa_level; fb.has_depth_buffer = has_depth_buffer;
}

void GfxRenderingAPIGX::StartDrawToFramebuffer(int fbId, float noiseScale) {
    (void)noiseScale;
    if (fbId < 0 || fbId >= (int)mFrameBuffers.size()) return;
    mCurrentFrameBuffer = (size_t)fbId;
}

void GfxRenderingAPIGX::CopyFramebuffer(int fbDstId, int fbSrcId,
                                        int srcX0, int srcY0, int srcX1, int srcY1,
                                        int dstX0, int dstY0, int dstX1, int dstY1) {
    (void)fbDstId; (void)fbSrcId;
    (void)srcX0; (void)srcY0; (void)srcX1; (void)srcY1;
    (void)dstX0; (void)dstY0; (void)dstX1; (void)dstY1;
}

void GfxRenderingAPIGX::ClearFramebuffer(bool color, bool depth) {
    if (color) GX_SetCopyClear({0, 0, 0, 255}, 0x00FFFFFF);
    if (depth) GX_SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
}

void GfxRenderingAPIGX::ClearDepthRegion(int x, int y, int w, int h) {
    (void)x; (void)y; (void)w; (void)h;
    ClearFramebuffer(false, true);
}

void GfxRenderingAPIGX::ReadFramebufferToCPU(int fbId, uint32_t width,
                                              uint32_t height, uint16_t* rgba16Buf) {
    (void)fbId; (void)width; (void)height; (void)rgba16Buf;
}

void GfxRenderingAPIGX::ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) {
    (void)fbIdTarger; (void)fbIdSrc;
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIGX::GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    (void)fb_id; (void)coordinates;
    return {};
}

void* GfxRenderingAPIGX::GetFramebufferTextureId(int fbId) { (void)fbId; return nullptr; }
void  GfxRenderingAPIGX::SelectTextureFb(int fbId) { (void)fbId; }

} // namespace Fast
#endif // ENABLE_GX
