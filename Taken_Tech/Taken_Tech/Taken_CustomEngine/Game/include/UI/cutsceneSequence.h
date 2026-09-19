#pragma once
/**
 * @file      cutsceneSequence.h
 * @author    Woh Kye Le
 * @email     w.kyele
 * @date      2026-04-03
 *
 * @brief     Declares CutsceneSequence, a lightweight timed slideshow for full-screen UI cutscenes.
 *
 * @version   1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */

#include <functional>
#include <string>
#include <vector>

#include "Graphics/mesh2d.h"

class Renderer;

/**
 * @brief Draws a full-screen fade overlay and manages a simple fade-in timer.
 *
 * This helper is used for screen transitions such as fading from black into
 * gameplay after an intro cutscene.
 */
class FadeOverlay
{
public:
    FadeOverlay() = default;

    /**
     * @brief Initializes internal mesh data for a full-screen quad.
     */
    void Init();
    /**
     * @brief Starts a fade-in from black into the underlying scene.
     * @param seconds Duration in seconds for the fade to complete.
     */
    void StartFadeIn(float seconds);
    /**
     * @brief Advances fade timing.
     * @param dt Delta time in seconds.
     */
    void Update(double dt);
    /**
     * @brief Draws the overlay using the current alpha.
     * @param renderer Renderer used to draw the overlay.
     */
    void Draw(Renderer& renderer) const;

private:
    Mesh2D mesh;
    float alpha = 0.0f;
    float duration = 0.0f;
};

/**
 * @brief Plays a sequence of full-screen textures with a fixed duration per slide.
 *
 * The sequence advances automatically based on Update() delta time and invokes a
 * completion callback when the last slide finishes.
 */
class CutsceneSequence
{
public:
    CutsceneSequence() = default;

    /**
     * @brief Prepares a cutscene sequence from texture keys.
     * @param textureKeys Logical keys from textures.json.
     * @param secondsPerSlide Duration of each slide in seconds.
     * @param onFinished Callback fired once when the sequence ends.
     */
    void Init(std::vector<std::string> textureKeys, float secondsPerSlide, std::function<void()> onFinished);

    /**
     * @brief Loads cutscene slides from a json file.
     * @param path Json file path relative to the asset root.
     * @param onFinished Callback fired once when the sequence ends.
     * @return True if the config was loaded successfully.
     */
    bool LoadConfig(const std::string& path, std::function<void()> onFinished);

    /**
     * @brief Loads cutscene slides from a named section inside a json file.
     * @param path Json file path relative to the asset root.
     * @param section Root object key containing the cutscene config.
     * @param onFinished Callback fired once when the sequence ends.
     * @return True if the config was loaded successfully.
     */
    bool LoadConfig(const std::string& path, const std::string& section, std::function<void()> onFinished);

    /**
     * @brief Starts playback from the first slide.
     */
    void Start();
    /**
     * @brief Stops playback and resets internal playback state.
     */
    void Stop();
    /**
     * @brief Returns true if the sequence is currently playing.
     * @return True when actively playing.
     */
    bool IsActive() const { return active; }

    /**
     * @brief Advances playback time and progresses slides.
     * @param dt Delta time in seconds.
     */
    void Update(double dt);
    /**
     * @brief Draws the current slide as a full-screen quad.
     * @param renderer Renderer used to draw the slide.
     */
    void Draw(Renderer& renderer) const;

    /**
     * @brief Returns configured fade-in duration to apply after the sequence completes.
     * @return Fade-in duration in seconds (0 if not configured).
     */
    float GetFadeInSecondsAfter() const { return fadeInSecondsAfter; }

    void SetFirstSlideZoomRate(float ratePerSec) { firstSlideZoomRatePerSec = ratePerSec; }

private:
    void BuildFullscreenQuad();

    Mesh2D bgMesh;
    struct Slide
    {
        unsigned tex = 0;
        float seconds = 0.0f;
        float fadeInSeconds = 0.0f;
        float fadeOutSeconds = 0.0f;
    };
    std::vector<Slide> slides;
    std::function<void()> onFinished;

    float fadeInSecondsAfter = 0.0f;
    float firstSlideZoomRatePerSec = 0.0f;
    bool active = false;
    int index = 0;
    double elapsed = 0.0;
};
