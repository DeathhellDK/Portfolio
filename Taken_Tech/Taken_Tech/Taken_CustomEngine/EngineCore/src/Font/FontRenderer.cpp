/**
 * @file    FontRenderer.cpp
 * @author  Low Jianlin
 * @email   jianlin.low ,t.weiliangterril
 * @co-author Tan Wei Liang Terril
 * @date    2025-11-07
 *
 * @brief   Implements the EngineCore::FontRenderer class.
 *
 * Bridges FreeType text shaping to OpenGL rendering. Handles:
 *  - Loading TrueType / OTF fonts into resident memory
 *  - Creating and updating A8 (alpha-only) GL texture atlases
 *  - Building per-glyph triangle geometry for UI text
 *  - Converting text positions from bottom-left to top-left coordinate space
 *
 * Uses:
 *  - FreeType for glyph rasterization
 *  - FontSys::Face for packing glyphs into atlases
 *  - Renderer.DrawMesh() to submit the resulting triangles.
 *
 * @version 1.1
 */
#include "Font/FontRenderer.h"
#include <unordered_map>
#include "Graphics/mesh2d.h"
#include "Math/matrix3x3.h"
#include "Math/vect3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Input/DebugConsole.hpp"

using namespace EngineCore;
using GLTex = GLuint;

namespace {
    FontSys::Library ft;
    FontSys::TextureHooks hooks;
    std::unordered_map<std::string, std::vector<uint8_t>> fontBytes;
    std::unordered_map<std::string, std::unique_ptr<FontSys::Face>> faces;
    bool inited = false;
}


/**
 * @brief Creates a GL texture in single-channel A8 format via atlas builder.
 *
 * @param w Width in pixels (rasterized glyph width)
 * @param h Height in pixels (rasterized glyph height)
 * @param px Pointer to alpha bitmap
 * @return GL texture ID cast to FontSys::TextureHandle
 */
FontSys::TextureHandle Engine_CreateTextureA8(int w, int h, const uint8_t* px)
{
    GLTex tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // 1-byte per pixel

    glTexImage2D(GL_TEXTURE_2D, 0,
        GL_RED,        // single channel
        w, h, 0,
        GL_RED, GL_UNSIGNED_BYTE,
        px);

    //GLint swizzle[] = { GL_RED, GL_RED, GL_RED, GL_RED };
    GLint swizzle[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return (FontSys::TextureHandle)tex;
}

/**
 * @brief Updates an existing texture atlas region with fresh glyph pixels.
 *
 * Typically called when a new codepoint is rasterized and packed.
 *
 * @param t Texture atlas to update
 * @param x Sub-region x offset
 * @param y Sub-region y offset
 * @param w Width of updated area
 * @param h Height of updated area
 * @param px Raw pixel buffer (A8)
 */
void Engine_UpdateTextureA8(FontSys::TextureHandle t,
    int x, int y, int w, int h,
    const uint8_t* px)
{
    GLTex tex = (GLTex)t;
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexSubImage2D(GL_TEXTURE_2D, 0,
        x, y,
        w, h,
        GL_RED, GL_UNSIGNED_BYTE,
        px);
}
//extern void Engine_DrawGlyphTris(FontSys::TextureHandle tex, const FontSys::GlyphVertex* verts, int count);

/**
 * @brief Initializes FreeType and GL texture hook bindings.
 *
 * Must be called before RegisterFont() or DrawText().
 */
void FontRenderer::Init() {
    if (inited) return;
    hooks.CreateTextureA8 = [](int w, int h, const uint8_t* p) {
        return Engine_CreateTextureA8(w, h, p);
        };
    hooks.UpdateTextureA8 = [](FontSys::TextureHandle t, int x, int y, int w, int h, const uint8_t* p) {
        Engine_UpdateTextureA8(t, x, y, w, h, p);
        };
    inited = true;
}


/**
 * @brief Clears cached font faces and raw loaded TTF/OTF byte buffers.
 *
 * Does not destroy GL textures (owned by GA/renderer).
 */
void FontRenderer::Shutdown() {
    faces.clear();
    fontBytes.clear();
    inited = false;
}

/**
 * @brief Loads a font file from disk into memory.
 *
 * @param name Runtime identifier (e.g. "default", "titleFont")
 * @param path OS path to a .ttf/.otf file
 *
 */
void FontRenderer::RegisterFont(const std::string& name, const std::string& path) {
    fontBytes[name] = FontSys::LoadFile(path.c_str());
}


/**
 * @brief Renders formatted UTF-8 string to screen-space UI.
 *
 * Designed for ImGui-like overlay text — camera is disabled temporarily.
 *
 * Pipeline:
 * 1) Lookup (or create) cached Face
 * 2) Use FreeType to rasterize needed codepoints
 * 3) Build triangle mesh from glyph quads (6 verts per glyph)
 * 4) Convert bottom-left text coordinates to top-left UI coordinate system
 * 5) Render via Renderer.DrawMesh() using atlas texture
 *
 * @param renderer Rendering API façade
 * @param fontName Font previously registered with RegisterFont()
 * @param size Pixel height of glyphs
 * @param x Pixel-space anchor (bottom-left horizontal)
 * @param y Pixel-space anchor (bottom-left vertical)
 * @param rgba Packed 0xRRGGBBAA tint
 * @param text UTF-8 content to display
 * @param wrapWidth Optional word-wrap width (0 = disabled)
 * @param align Horizontal alignment (currently unused)
 *
 * @warning Requires valid OpenGL context and active window.
 * @warning Does not support rotation / scaling (UI text only).
 */
void FontRenderer::DrawText(Renderer& renderer,
    const std::string& fontName, float size,
    float x, float y, uint32_t rgba,
    std::string_view text,
    float wrapWidth, FontSys::Align align)
{
    // --- Font cache lookup / creation ---
    // Use size in key to support multiple sizes for the same font
    std::string key = fontName + ":" + std::to_string((int)std::round(size));

    auto itFace = faces.find(key);
    if (itFace == faces.end()) {
        auto fb = fontBytes.find(fontName);
        if (fb == fontBytes.end()) return;

        auto face = std::make_unique<FontSys::Face>(
            ft, hooks, fb->second.data(), fb->second.size(), size);
        itFace = faces.emplace(key, std::move(face)).first;
    }
    auto& face = *itFace->second;

    // We need window and framebuffer size to handle high-DPI scaling
    GLFWwindow* win = glfwGetCurrentContext();
    int ww, wh; glfwGetWindowSize(win, &ww, &wh);
    int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);

    float ratioX = (float)fbw / (float)ww;
    float ratioY = (float)fbh / (float)wh;

    // Caller supplies coordinates in window pixels. 
    // BuildText expects pixels that match the font size (framebuffer pixels).
    float x_fb = x * ratioX;
    float y_fb = (static_cast<float>(wh) - y) * ratioY; // flip Y and scale to FB
    float wrapWidth_fb = wrapWidth * ratioX;

    // Build text: triangle list (6 verts per glyph), already UV-mapped to atlas
    auto dc = face.buildText(text, x_fb, y_fb, rgba, wrapWidth_fb, align);
    if (dc.verts.empty()) return;


    // --- Pack verts 1:1 ---
    const size_t N = dc.verts.size();
    std::vector<Vertex2D> verts; verts.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        const auto& gv = dc.verts[i];
        Vector3 col = {
            ((gv.rgba >> 24) & 0xFF) / 255.0f,
            ((gv.rgba >> 16) & 0xFF) / 255.0f,
            ((gv.rgba >> 8) & 0xFF) / 255.0f
        };
        verts.push_back({ {gv.x, gv.y}, {gv.u, gv.v}, col });
    }

    // Trivial indices 0..N-1
    std::vector<unsigned int> inds(N);
    for (unsigned i = 0; i < N; ++i) inds[i] = i;

    Mesh2D mesh;
    mesh.Reserve(N, N);
    mesh.SetVertices(verts);
    mesh.SetIndices(inds);

    // --- Screen-space UI: disable camera ---
    Camera2D* oldCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    // Pixel -> NDC with top-left origin (using framebuffer size for consistency with dc.verts)
    float sx = 2.0f / (float)fbw;
    float sy = -2.0f / (float)fbh;
    float tx = -1.0f;
    float ty = 1.0f;

    Matrix3x3 model =
        Matrix3x3::BuildTranslation(tx, ty) *
        Matrix3x3::BuildScaling(sx, sy);

    // Neutralize sprite-sheet animation for fonts
    if (Shader* sh = renderer.GetShader()) {
        sh->SetInt("u_Frame", 0);
        sh->SetInt("u_Cols", 1);
        sh->SetVec2("u_FrameSize", { 1.0f, 1.0f });
    }

    Vector3 tint = { 1,1,1 };
    renderer.DrawMesh(mesh, model, tint, (GLuint)dc.texture);

    /*DebugConsole::Get().AddFormattedMessage(LogLevel::Info, "[FontRenderer] Using font '" + fontName
          ,"' -> GL texture ID: " + std::to_string((GLuint)dc.texture), "\n");*/

    renderer.setCamera(oldCam);
}





/**
 * @brief Computes the pixel width of a text string using a specified font and size.
 *
 * This function calculates the total horizontal extent (width) of the given text
 * when rendered with the specified font and size. It handles font loading and
 * glyph generation internally without actually drawing the text.
 *
 * @param fontName The name of the font to use for measurement. Must be a loaded font.
 * @param size The font size in pixels for the measurement.
 * @param text The text string to measure. Uses string_view for efficient string handling.
 *
 * @return The width of the text in pixels. Returns 0.0f if:
 *         - The specified font is not found
 *         - The text generates no vertices (empty text or invalid font)
 *         - The font cannot be loaded from the font bytes cache
 *
 * @note This function performs font face loading and glyph generation if the
 *       requested font isn't already cached. The measurement is based on the
 *       actual glyph geometry and includes proper kerning and spacing.
 * @note The returned width represents the actual pixel space extent from the
 *       leftmost to rightmost point of the rendered text bounds.
 *
 * @see FontSys::Face::buildText
 * @see FontRenderer
 *
 * @example
 * float width = FontRenderer::ComputeTextWidth("Arial", 24.0f, "Hello World");
 * // width now contains the pixel width of "Hello World" in 24pt Arial
 */
float FontRenderer::ComputeTextWidth( const std::string& fontName, float size, std::string_view text){
    // Lookup / load font face
    std::string key = fontName + ":" + std::to_string((int)std::round(size));

    auto itFace = faces.find(key);
    if (itFace == faces.end()) {
        auto fb = fontBytes.find(fontName);
        if (fb == fontBytes.end())
            return 0.f;

        auto face = std::make_unique<FontSys::Face>(
            ft, hooks, fb->second.data(), fb->second.size(), size);
        itFace = faces.emplace(key, std::move(face)).first;
    }
    auto& face = *itFace->second;

    // Build glyphs but don’t draw
    auto dc = face.buildText(text, 0.f, 0.f, 0xFFFFFFFF, 0.f, FontSys::Align::Left);

    if (dc.verts.empty())
        return 0.f;

    float minX = dc.verts[0].x;
    float maxX = dc.verts[0].x;

    for (auto& v : dc.verts) {
        minX = std::min(minX, v.x);
        maxX = std::max(maxX, v.x);
    }

    return maxX - minX;   // text width in *pixel space*
}

/**
    * @brief Computes the full pixel-height of a rendered UTF-8 string.
    *
    * Similar to ComputeTextWidth(), but scans all glyph vertices to find
    * the vertical min/max pixel bounds.
    *
    * @param font  Font name to measure.
    * @param size  Pixel size of glyphs.
    * @param text  UTF-8 string to measure.
    *
    * @return Text height in pixels (maxY - minY). Returns 0 if vertex list is empty.
    *
    * @note Uses the already-loaded FontSys::Face; the font must exist in the cache.
*/
float FontRenderer::ComputeTextHeight(const std::string& fontName, float size, std::string_view text){
    std::string key = fontName + ":" + std::to_string((int)std::round(size));

    // Ensure face exists
    auto itFace = faces.find(key);
    if (itFace == faces.end()) {
        auto fb = fontBytes.find(fontName);
        if (fb == fontBytes.end())
            return 0.f;

        auto face = std::make_unique<FontSys::Face>(
            ft, hooks, fb->second.data(), fb->second.size(), size);
        itFace = faces.emplace(key, std::move(face)).first;
    }
    
    // build glyphs (not drawn)
    auto dc = itFace->second->buildText(text, 0.f, 0.f, 0xFFFFFFFF, 0.f, FontSys::Align::Left);
    if (dc.verts.empty()) return 0.f;

    float minY = dc.verts[0].y;
    float maxY = dc.verts[0].y;

    for (auto& v : dc.verts) {
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
    }

    return maxY - minY;
}

/**
    * @brief Generates raw glyph geometry for advanced/custom text rendering.
    *
    * Does NOT draw text. Instead, it returns:
    * - The texture atlas used for the glyphs
    * - A vector of glyph vertices (position + UV + color)
    *
    * Useful for:
    * - Custom batching
    * - Signed-distance-field effects
    * - Shader-driven text animations
    *
    * @param fontName  Registered font name.
    * @param size      Pixel size of the font.
    * @param text      UTF-8 string to layout.
    * @param wrapWidth Optional wrapping width in pixels.
    * @param align     Alignment mode.
    *
    * @return TextGeometry struct containing:
    *         - GL texture handle of the atlas
    *         - Vertex list of all glyphs
*/
FontRenderer::TextGeometry FontRenderer::BuildTextGeometry(const std::string& fontName, float size, std::string_view text, float wrapWidth, FontSys::Align align){
    TextGeometry tg;

    // Load face if not already loaded
    std::string key = fontName + ":" + std::to_string((int)std::round(size));

    auto itFace = faces.find(key);
    if (itFace == faces.end()) {
        auto fb = fontBytes.find(fontName);
        if (fb == fontBytes.end())
            return tg;

        auto face = std::make_unique<FontSys::Face>(
            ft, hooks, fb->second.data(), fb->second.size(), size);
        itFace = faces.emplace(key, std::move(face)).first;
    }

    auto& face = *itFace->second;

    // Build text at (0,0) but wrapped
    auto dc = face.buildText(text, 0.f, 0.f, 0xFFFFFFFF, wrapWidth, align);

    tg.texture = dc.texture;
    tg.verts = std::move(dc.verts);
    return tg;
}