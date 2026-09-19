#pragma once
/**
 * @file      FontSystem.h
 * @author    Low Jianlin
 * @email     
 * @date      2025-11-07
 *
 * @brief     Core font rendering backend built on FreeType.
 *
 * Provides:
 * - Font loading from memory
 * - UTF-8 decoding
 * - Glyph caching + atlas packing
 * - CPU-side text mesh building
 * - Texture updates via customizable callbacks
 *
 * Rendering backends must supply texture creation/update hooks.
 *
 * @version   1.1
 */
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRUETYPE_TABLES_H

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <memory>
#include <cassert>
#include <cmath>

namespace FontSys {

    // ----------------------------- Engine bridges ------------------------------
    using TextureHandle = uint64_t;

    /**
    * @struct TextureHooks
    * @brief Callback interface supplying texture allocation and updates.
    *
    * FontSystem is renderer-agnostic, so texture binding must be provided
    * externally. Uses A8 (alpha-only) pixel format.
    */
    struct TextureHooks {
        std::function<TextureHandle(int, int, const uint8_t*)> CreateTextureA8;
        std::function<void(TextureHandle, int, int, int, int, const uint8_t*)> UpdateTextureA8;
    };

    // ------------------------------ Vertex format ------------------------------
    /**
    * @struct GlyphVertex
    * @brief Basic vertex format for CPU-generated text quads.
    */
    struct GlyphVertex {
        float x, y;
        float u, v;
        uint32_t rgba;
    };

    /**
    * @struct DrawCall
    * @brief Output batch: one texture per draw call.
    */
    struct DrawCall {
        TextureHandle texture = 0;
        std::vector<GlyphVertex> verts;
    };

    // ------------------------------- UTF-8 decode -------------------------------
    /**
     * @brief Decodes the next UTF-8 codepoint from a buffer.
     *
     * @param it  [in/out] Iterator pointing into byte stream.
     * @param end Byte end pointer.
     * @param cp  Output decoded Unicode codepoint.
     * @return True if a codepoint was produced, false if no bytes left.
     */
    inline bool utf8Decode(const char*& it, const char* end, uint32_t& cp) {
        if (it >= end) return false;
        unsigned char c = (unsigned char)*it++;
        if (c < 0x80) { cp = c; return true; }
        if ((c >> 5) == 0x6) { unsigned char c1 = (unsigned char)*it++; cp = ((c & 0x1F) << 6) | (c1 & 0x3F); return true; }
        if ((c >> 4) == 0xE) { unsigned char c1 = (unsigned char)*it++; unsigned char c2 = (unsigned char)*it++; cp = ((c & 0xF) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F); return true; }
        if ((c >> 3) == 0x1E) { unsigned char c1 = (unsigned char)*it++; unsigned char c2 = (unsigned char)*it++; unsigned char c3 = (unsigned char)*it++; cp = ((c & 0x7) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F); return true; }
        cp = 0xFFFD; return true;
    }

    // ------------------------------- FreeType RAII ------------------------------
    /**
     * @class Library
     * @brief RAII wrapper for `FT_Library` lifetime.
     */
    class Library {
    public:
        /** @brief Initializes the FreeType library. */
        Library() { FT_Init_FreeType(&lib_); }
        ~Library() { FT_Done_FreeType(lib_); }
        
        /** @brief Gets the underlying FT_Library handle. */
        FT_Library get() const { return lib_; }
    private:
        FT_Library lib_{};
    };

    // ----------------------------- Atlas / Packing ------------------------------
    struct AtlasRect { int x{}, y{}, w{}, h{}; };

    /**
     * @class Atlas
     * @brief Simple shelf-packing alpha atlas for cached glyphs.
     */
    class Atlas {
    public:
        /**
         * @param hooks Texture API callbacks
         * @param w Initial width of atlas in pixels
         * @param h Initial height of atlas in pixels
         */
        Atlas(TextureHooks hooks, int w = 256, int h = 256)
            : hooks_(std::move(hooks)), width_(w), height_(h), pixels_(w* h, 0) {
            texture_ = hooks_.CreateTextureA8(width_, height_, pixels_.data());
        }

        /** @brief Returns the texture handle assigned to this atlas. */
        TextureHandle handle() const { return texture_; }
        
        /** @brief Gets the width of the atlas texture. */
        int width() const { return width_; }
        
        /** @brief Gets the height of the atlas texture. */
        int height() const { return height_; }

        /**
         * @brief Allocates space inside atlas using shelf-packing strategy.
         * @param gw Width of the glyph to pack.
         * @param gh Height of the glyph to pack.
         * @param out Reference to an AtlasRect to store the assigned coordinates.
         * @return True if allocation succeeded, false if the atlas is full.
         */
        bool allocate(int gw, int gh, AtlasRect& out) {
            const int w = gw + 1, h = gh + 1;
            if (cursorX_ + w > width_) { cursorX_ = 0; shelfY_ += shelfH_; shelfH_ = 0; }
            if (shelfY_ + h > height_) return false;
            out.x = cursorX_; out.y = shelfY_; out.w = gw; out.h = gh;
            cursorX_ += w; shelfH_ = std::max(shelfH_, h);
            return true;
        }

        /**
         * @brief Writes bitmap data into atlas and triggers a texture refresh.
         * @param r Target rectangle in the atlas to write to.
         * @param src Pointer to the raw glyph bitmap data.
         * @param pitch Byte pitch (row width) of the source bitmap.
         */
        void blit(const AtlasRect& r, const uint8_t* src, int pitch) {
            for (int row = 0; row < r.h; ++row) {
                uint8_t* dst = &pixels_[(r.y + row) * width_ + r.x];
                const uint8_t* s = src + row * pitch;
                std::memcpy(dst, s, r.w);
            }
            hooks_.UpdateTextureA8(texture_, 0, 0, width_, height_, pixels_.data());
        }

    private:
        TextureHooks hooks_;
        TextureHandle texture_{};
        int width_, height_;
        std::vector<uint8_t> pixels_;
        int cursorX_ = 0, shelfY_ = 0, shelfH_ = 0;
    };

    // ------------------------------- Font Face ---------------------------------
    struct Glyph {
        AtlasRect rect;
        int advance = 0, bearingX = 0, bearingY = 0, width = 0, height = 0;
    };

    struct FaceMetrics {
        float ascent = 0.f, descent = 0.f, lineGap = 0.f;
        
        /** @brief Calculates the total line height (ascent + descent + lineGap). */
        float lineHeight() const { return ascent + descent + lineGap; }
    };

    /**
     * @enum Align
     * @brief Horizontal alignment options for text layout.
     */
    enum class Align { Left, Center, Right };

    /**
    * @class Face
    * @brief Represents a loaded font face and its cached glyphs.
    */
    class Face {
    public:
        Face(Library& lib, TextureHooks hooks,
            const void* fontBytes, size_t fontSize,
            float pixelHeight)
            : hooks_(std::move(hooks)), atlas_(hooks_), pixelHeight_(pixelHeight)
        {
            FT_New_Memory_Face(lib.get(), (const FT_Byte*)fontBytes, (FT_Long)fontSize, 0, &face_);
            FT_Set_Pixel_Sizes(face_, 0, (FT_UInt)std::round(pixelHeight_));
            metrics_.ascent = face_->size->metrics.ascender / 64.f;
            metrics_.descent = -face_->size->metrics.descender / 64.f;
            metrics_.lineGap = (face_->size->metrics.height - face_->size->metrics.ascender + face_->size->metrics.descender) / 64.f;
        }

        ~Face() { FT_Done_Face(face_); }
        const FaceMetrics& metrics() const { return metrics_; }
        TextureHandle atlasTexture() const { return atlas_.handle(); }

        const Glyph& getGlyph(uint32_t codepoint) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = glyphs_.find(codepoint);
            if (it != glyphs_.end()) return it->second;

            FT_Load_Char(face_, codepoint, FT_LOAD_RENDER);
            FT_GlyphSlot slot = face_->glyph;

            Glyph g{};
            g.advance = (int)std::lround(slot->advance.x / 64.0);
            g.bearingX = slot->bitmap_left;
            g.bearingY = slot->bitmap_top;
            g.width = slot->bitmap.width;
            g.height = slot->bitmap.rows;

            if (g.width && g.height) {
                AtlasRect r{}; atlas_.allocate(g.width, g.height, r);
                atlas_.blit(r, slot->bitmap.buffer, slot->bitmap.pitch);
                g.rect = r;
            }
            glyphs_[codepoint] = g;
            return glyphs_[codepoint];
        }

       /* DrawCall buildText(std::string_view text, float x, float y, uint32_t color,
            float wrapWidth = 0.f, Align align = Align::Left) {
            (void)wrapWidth;
            (void)align;
            DrawCall dc; dc.texture = atlas_.handle();
            float penX = x, penY = y;
            const char* it = text.data(); const char* end = it + text.size();
            while (it < end) {
                uint32_t cp; utf8Decode(it, end, cp);
                if (cp == '\\n') { penY += metrics_.lineHeight(); penX = x; continue; }
                const Glyph& g = getGlyph(cp);
                float gx = penX + (float)g.bearingX, gy = penY - (float)g.bearingY;
                float gw = (float)g.width, gh = (float)g.height;
                float u0 = (float)g.rect.x / atlas_.width(), v0 = (float)g.rect.y / atlas_.height();
                float u1 = (float)(g.rect.x + g.rect.w) / atlas_.width(), v1 = (float)(g.rect.y + g.rect.h) / atlas_.height();
                auto push = [&](float X, float Y, float U, float V) {dc.verts.push_back({ X,Y,U,V,color }); };
                push(gx, gy, u0, v0); push(gx + gw, gy, u1, v0); push(gx + gw, gy + gh, u1, v1);
                push(gx, gy, u0, v0); push(gx + gw, gy + gh, u1, v1); push(gx, gy + gh, u0, v1);
                penX += g.advance;
            }
            return dc;
        }*/

        DrawCall buildText(std::string_view text, float x, float y, uint32_t color,
            float wrapWidth = 0.f, Align align = Align::Left)
        {
            DrawCall dc; dc.texture = atlas_.handle();
            const char* it = text.data(); const char* end = it + text.size();

            struct Line { std::vector<uint32_t> cps; float width = 0.f; };
            std::vector<Line> lines; Line cur;
            float penX = 0.f, penY = y, lh = metrics_.lineHeight();

            while (it < end) {
                uint32_t cp; utf8Decode(it, end, cp);
                if (cp == '\n') { cur.width = penX; lines.push_back(cur); cur = Line{}; penX = 0.f; continue; }
                const Glyph& g = getGlyph(cp);
                if (wrapWidth > 0.f && penX + g.advance > wrapWidth) {
                    cur.width = penX; lines.push_back(cur); cur = Line{}; penX = 0.f;
                }
                cur.cps.push_back(cp); penX += g.advance;
            }
            cur.width = penX; lines.push_back(cur);

            for (auto& ln : lines) {
                float ox = 0.f;
                if (align == Align::Center) ox = (wrapWidth - ln.width) * 0.5f;
                else if (align == Align::Right) ox = (wrapWidth - ln.width);
                float px = x + ox;

                for (uint32_t cp : ln.cps) {
                    const Glyph& g = getGlyph(cp);
                    float gx = px + g.bearingX, gy = penY - g.bearingY;
                    float gw = (float)g.width, gh = (float)g.height;
                    float u0 = (float)g.rect.x / atlas_.width(), v0 = (float)g.rect.y / atlas_.height();
                    float u1 = (float)(g.rect.x + g.rect.w) / atlas_.width(), v1 = (float)(g.rect.y + g.rect.h) / atlas_.height();
                    auto push = [&](float X, float Y, float U, float V) { dc.verts.push_back({ X,Y,U,V,color }); };
                    push(gx, gy, u0, v0); push(gx + gw, gy, u1, v0); push(gx + gw, gy + gh, u1, v1);
                    push(gx, gy, u0, v0); push(gx + gw, gy + gh, u1, v1); push(gx, gy + gh, u0, v1);
                    px += g.advance;
                }
                penY += lh;
            }
            return dc;
        }


    private:
        TextureHooks hooks_;
        Atlas atlas_;
        FT_Face face_{};
        float pixelHeight_{};
        FaceMetrics metrics_{};
        std::unordered_map<uint32_t, Glyph> glyphs_;
        std::mutex mutex_;
    };

    // ------------------------------ File helper --------------------------------
    /**
     * @brief Loads an entire file into a byte buffer.
     * @return Byte vector containing file contents, or empty if failed.
     */
    inline std::vector<uint8_t> LoadFile(const char* path) {
        FILE* f = nullptr;
#if defined(_MSC_VER)
        fopen_s(&f, path, "rb");
#else
        f = fopen(path, "rb");
#endif
        if (!f) return {};
        fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
        std::vector<uint8_t> buf(n); fread(buf.data(), 1, n, f); fclose(f);
        return buf;
    }
}