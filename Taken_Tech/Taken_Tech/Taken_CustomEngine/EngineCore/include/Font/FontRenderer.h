#pragma once
/**
 * @file      fontrenderer.h
 * @author    Low Jianlin
 * @email     
 * @date      2025-11-07
 *
 * @brief     Declares the FontRenderer utility for text rendering.
 *
 * The FontRenderer provides a lightweight, centralized interface for
 * managing and drawing text using the underlying FontSystem and Renderer.
 *
 * Responsibilities:
 * - Initialize and shut down the font subsystem.
 * - Register fonts by name and file path.
 * - Render text with specified alignment, color, and wrapping.
 *
 * @version   1.0
 * @namespace EngineCore
 * @copyright
 * Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without prior
 * written consent of DigiPen Institute of Technology is prohibited.
 */
#include "Font/FontSystem.h"
#include <string>
#include "Graphics/renderer.h"

namespace EngineCore {
    /**
     * @class FontRenderer
     * @brief Static utility for rendering text through the FontSystem.
     *
     * FontRenderer acts as a convenience layer between the rendering system
     * and the font subsystem. It simplifies font loading and text rendering
     * for both in-game UI and debugging overlays.
     *
     * Example usage:
     * EngineCore::FontRenderer::Init();
     * EngineCore::FontRenderer::RegisterFont("Main", "Assets/fonts/Vogue.ttf");
     * EngineCore::FontRenderer::DrawText(renderer, "Main", 24.0f, 100, 200,
     *                                    0xFFFFFFFF, "Hello World!");
     */
    class FontRenderer {
    public:

        struct TextGeometry {
            std::vector<FontSys::GlyphVertex> verts;
            FontSys::TextureHandle texture = 0;
        };

        /**
        * @brief Initializes the font rendering subsystem.
        *
        * Must be called before using any other FontRenderer methods.
        * Internally sets up necessary font atlases and buffers.
        */
        static void Init();
        /**
         * @brief Releases all font resources and shuts down the subsystem.
         *
         * Should be called once during engine shutdown to ensure clean
         * resource dealloc.
         */
        static void Shutdown();
        /**
         * @brief Registers a new font for use in rendering.
         *
         * Associates a human-readable name with a font file path so that
         * later draw calls can reference it easily.
         *
         * @param name Unique name identifier for the font (e.g., "MainUI").
         * @param path File path to the font (e.g., "Assets/fonts/Roboto.ttf").
         */
        static void RegisterFont(const std::string& name, const std::string& path);

        /**
         * @brief Draws text on screen using the specified font.
         *
         * Renders a UTF-8 encoded text string using a registered font,
         * applying color, size, alignment, and optional word wrapping.
         *
         * @param renderer Reference to the Renderer used for submitting draw calls.
         * @param fontName Registered font name to use for rendering.
         * @param size Font size in pixels.
         * @param x X position (in screen or world coordinates depending on render mode).
         * @param y Y position.
         * @param rgba 32-bit color value (RGBA format).
         * @param text The UTF-8 string to draw.
         * @param wrapWidth Optional maximum width before wrapping text (default: 0, no wrap).
         * @param align Horizontal alignment mode (Left, Center, Right).
         */
        static void DrawText(Renderer& renderer,
            const std::string& fontName, float size,
            float x, float y, uint32_t rgba,
            std::string_view text,
            float wrapWidth = 0.f,
            FontSys::Align align = FontSys::Align::Left);

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
        */
        static float ComputeTextWidth(const std::string& fontName, float size, std::string_view text);



        static float ComputeTextHeight(const std::string& font, float, std::string_view text);


        static TextGeometry BuildTextGeometry(const std::string& fontName, float size, std::string_view text, float wrapWidth, FontSys::Align align);
    };

    

} // namespace EngineCore