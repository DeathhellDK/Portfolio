/**
 * @file      cutsceneSequence.cpp
 * @author    Woh Kye Le
 * @email     w.kyele
 * @date      2026-04-03
 *
 * @brief     Implements CutsceneSequence timing and full-screen rendering.
 *
 * @version   1.0
 * @copyright
 * Copyright (C) 2026 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the
 * prior written consent of DigiPen Institute of Technology is prohibited.
 */
#include "UI/cutsceneSequence.h"

#include "Core/assetsPath.h"
#include "Core/resourceManager.h"
#include "Graphics/camera2d.h"
#include "Graphics/renderer.h"
#include "Graphics/vertex2d.h"
#include "Math/matrix3x3.h"
#include "Math/vect3.h"

#include <GLFW/glfw3.h>
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

/**
 * @brief Initializes internal mesh data for a full-screen quad.
 */
void FadeOverlay::Init()
{
    Vector3 white(1.f, 1.f, 1.f);
    std::vector<Vertex2D> verts = {
        Vertex2D(Vector2(0.f, 0.f), Vector2(0.f, 0.f), white),
        Vertex2D(Vector2(1.f, 0.f), Vector2(1.f, 0.f), white),
        Vertex2D(Vector2(1.f, 1.f), Vector2(1.f, 1.f), white),
        Vertex2D(Vector2(0.f, 1.f), Vector2(0.f, 1.f), white)
    };
    std::vector<unsigned int> inds = { 0,1,2, 0,2,3 };

    mesh.Reserve(verts.size(), inds.size());
    mesh.SetVertices(verts);
    mesh.SetIndices(inds);
}

/**
 * @brief Starts a fade-in from black into the underlying scene.
 * @param seconds Duration in seconds for the fade to complete.
 */
void FadeOverlay::StartFadeIn(float seconds)
{
    duration = seconds;
    if (duration <= 0.0f) duration = 1.0f;
    alpha = 1.0f;
}

/**
 * @brief Advances fade timing.
 * @param dt Delta time in seconds.
 */
void FadeOverlay::Update(double dt)
{
    if (alpha <= 0.0f) return;
    if (duration <= 0.0f) { alpha = 0.0f; return; }

    alpha -= (float)(dt / (double)duration);
    if (alpha < 0.0f) alpha = 0.0f;
}

/**
 * @brief Draws the overlay using the current alpha.
 * @param renderer Renderer used to draw the overlay.
 */
void FadeOverlay::Draw(Renderer& renderer) const
{
    if (alpha <= 0.0f) return;

    Camera2D* prevCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    const float prevAlpha = renderer.GetGlobalAlpha();
    renderer.SetGlobalAlpha(alpha);

    Matrix3x3 model =
        Matrix3x3::BuildTranslation(-1.f, -1.f) *
        Matrix3x3::BuildScaling(2.f, 2.f);
    renderer.DrawMesh(mesh, model, Vector3(0.f, 0.f, 0.f), 0);

    renderer.SetGlobalAlpha(prevAlpha);
    renderer.setCamera(prevCam);
}

/**
 * @brief Builds a full-screen quad mesh in normalized [0,1] space.
 */
void CutsceneSequence::BuildFullscreenQuad()
{
    Vector3 white(1.f, 1.f, 1.f);
    std::vector<Vertex2D> verts = {
        Vertex2D(Vector2(0.f, 0.f), Vector2(0.f, 0.f), white),
        Vertex2D(Vector2(1.f, 0.f), Vector2(1.f, 0.f), white),
        Vertex2D(Vector2(1.f, 1.f), Vector2(1.f, 1.f), white),
        Vertex2D(Vector2(0.f, 1.f), Vector2(0.f, 1.f), white)
    };
    std::vector<unsigned int> inds = { 0,1,2, 0,2,3 };

    bgMesh.Reserve(verts.size(), inds.size());
    bgMesh.SetVertices(verts);
    bgMesh.SetIndices(inds);
}

/**
 * @brief Prepares a cutscene sequence from texture keys with uniform slide duration.
 * @param textureKeys Logical keys from textures.json.
 * @param secondsPerSlide_ Duration of each slide in seconds.
 * @param onFinished_ Callback fired once when the sequence ends.
 */
void CutsceneSequence::Init(std::vector<std::string> textureKeys, float secondsPerSlide_, std::function<void()> onFinished_)
{
    BuildFullscreenQuad();

    slides.clear();
    slides.reserve(textureKeys.size());
    for (const auto& key : textureKeys) {
        Slide s;
        s.tex = ResourceManager::GetTexture(key);
        s.seconds = secondsPerSlide_;
        s.fadeInSeconds = 0.0f;
        s.fadeOutSeconds = 0.0f;
        if (s.tex != 0 && s.seconds > 0.0f) slides.push_back(s);
    }

    onFinished = std::move(onFinished_);
    fadeInSecondsAfter = 0.0f;
    firstSlideZoomRatePerSec = 0.0f;

    Stop();
}

/**
 * @brief Loads cutscene slides from a json file.
 * @param path Json file path relative to the asset root.
 * @param onFinished_ Callback fired once when the sequence ends.
 * @return True if the config was loaded successfully.
 */
bool CutsceneSequence::LoadConfig(const std::string& path, std::function<void()> onFinished_)
{
    return LoadConfig(path, std::string(), std::move(onFinished_));
}

/**
 * @brief Loads cutscene slides from a named section inside a json file.
 * @param path Json file path relative to the asset root.
 * @param section Root object key containing the cutscene config.
 * @param onFinished_ Callback fired once when the sequence ends.
 * @return True if the config was loaded successfully.
 */
bool CutsceneSequence::LoadConfig(const std::string& path, const std::string& section, std::function<void()> onFinished_)
{
    const std::string fullPath = AssetPath(path);

    std::ifstream ifs(fullPath);
    if (!ifs.is_open()) return false;

    json root;
    try { ifs >> root; }
    catch (...) { return false; }

    const json* cfg = &root;
    if (!section.empty()) {
        if (!root.contains(section) || !root[section].is_object()) return false;
        cfg = &root[section];
    }

    float defaultSeconds = 2.5f;
    if (cfg->contains("secondsPerSlide") && (*cfg)["secondsPerSlide"].is_number()) {
        defaultSeconds = (*cfg)["secondsPerSlide"].get<float>();
    }

    float defaultFadeOutSeconds = 0.0f;
    if (cfg->contains("fadeOutSeconds") && (*cfg)["fadeOutSeconds"].is_number()) {
        defaultFadeOutSeconds = (*cfg)["fadeOutSeconds"].get<float>();
    }

    float loadedFadeInSecondsAfter = 0.0f;
    if (cfg->contains("gameplayFadeInSeconds") && (*cfg)["gameplayFadeInSeconds"].is_number()) {
        loadedFadeInSecondsAfter = (*cfg)["gameplayFadeInSeconds"].get<float>();
    }

    std::vector<Slide> loaded;
    bool loadedFromSlides = false;

    if (cfg->contains("slides") && (*cfg)["slides"].is_array())
    {
        loadedFromSlides = true;
        for (const auto& it : (*cfg)["slides"])
        {
            if (!it.is_object()) continue;
            if (!it.contains("texture") || !it["texture"].is_string()) continue;

            Slide s;
            s.tex = ResourceManager::GetTexture(it["texture"].get<std::string>());
            s.seconds = defaultSeconds;
            if (it.contains("seconds") && it["seconds"].is_number()) s.seconds = it["seconds"].get<float>();
            s.fadeInSeconds = 0.0f;
            if (it.contains("fadeInSeconds") && it["fadeInSeconds"].is_number()) s.fadeInSeconds = it["fadeInSeconds"].get<float>();
            s.fadeOutSeconds = 0.0f;
            if (it.contains("fadeOutSeconds") && it["fadeOutSeconds"].is_number()) s.fadeOutSeconds = it["fadeOutSeconds"].get<float>();

            if (s.tex != 0 && s.seconds > 0.0f) loaded.push_back(s);
        }
    }
    else if (cfg->contains("textures") && (*cfg)["textures"].is_array())
    {
        for (const auto& it : (*cfg)["textures"])
        {
            if (!it.is_string()) continue;
            Slide s;
            s.tex = ResourceManager::GetTexture(it.get<std::string>());
            s.seconds = defaultSeconds;
            s.fadeInSeconds = 0.0f;
            s.fadeOutSeconds = 0.0f;
            if (s.tex != 0 && s.seconds > 0.0f) loaded.push_back(s);
        }
    }
    else
    {
        return false;
    }

    if (defaultFadeOutSeconds > 0.0f && !loaded.empty())
    {
        Slide& last = loaded.back();
        if (!loadedFromSlides || last.fadeOutSeconds <= 0.0f) {
            last.fadeOutSeconds = defaultFadeOutSeconds;
        }
    }

    BuildFullscreenQuad();
    slides = std::move(loaded);
    onFinished = std::move(onFinished_);
    fadeInSecondsAfter = loadedFadeInSecondsAfter;
    firstSlideZoomRatePerSec = 0.0f;
    Stop();
    return true;
}

/**
 * @brief Starts playback from the first slide.
 */
void CutsceneSequence::Start()
{
    if (slides.empty()) {
        active = false;
        if (onFinished) onFinished();
        return;
    }

    active = true;
    index = 0;
    elapsed = 0.0;
}

/**
 * @brief Stops playback and resets internal playback state.
 */
void CutsceneSequence::Stop()
{
    active = false;
    index = 0;
    elapsed = 0.0;
}

/**
 * @brief Advances playback time and progresses slides.
 * @param dt Delta time in seconds.
 */
void CutsceneSequence::Update(double dt)
{
    if (!active) return;

    elapsed += dt;
    if (index < 0 || index >= (int)slides.size()) return;
    const double currentSeconds = (double)slides[index].seconds;
    if (currentSeconds <= 0.0) return;
    if (elapsed < currentSeconds) return;

    elapsed = 0.0;
    index++;

    if (index >= (int)slides.size()) {
        active = false;
        if (onFinished) onFinished();
    }
}

/**
 * @brief Draws the current slide as a full-screen quad.
 * @param renderer Renderer used to draw the slide.
 */
void CutsceneSequence::Draw(Renderer& renderer) const
{
    if (!active) return;
    if (index < 0 || index >= (int)slides.size()) return;

    Camera2D* prevCam = renderer.getCamera();
    renderer.setCamera(nullptr);

    const float prevAlpha = renderer.GetGlobalAlpha();
    renderer.SetGlobalAlpha(1.0f);

    Matrix3x3 model =
        Matrix3x3::BuildTranslation(-1.f, -1.f) *
        Matrix3x3::BuildScaling(2.f, 2.f);
    if (index == 0 && firstSlideZoomRatePerSec > 0.0f) {
        const float z = 1.0f + firstSlideZoomRatePerSec * (float)elapsed;
        model = Matrix3x3::BuildScaling(z, z) * model;
    }

    renderer.DrawMesh(bgMesh, model, Vector3(0.f, 0.f, 0.f), 0);

    float slideAlpha = 1.0f;
    {
        float fadeInSeconds = slides[index].fadeInSeconds;
        if (fadeInSeconds > slides[index].seconds) fadeInSeconds = slides[index].seconds;
        if (fadeInSeconds > 0.0f) {
            float t = (float)elapsed / fadeInSeconds;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            slideAlpha = t * t;
        }
    }
    {
        const float slideSeconds = slides[index].seconds;
        float fadeOutSeconds = slides[index].fadeOutSeconds;
        if (fadeOutSeconds > slideSeconds) fadeOutSeconds = slideSeconds;
        if (fadeOutSeconds > 0.0f) {
            const float startFade = slideSeconds - fadeOutSeconds;
            if ((float)elapsed > startFade) {
                float t = ((float)elapsed - startFade) / fadeOutSeconds;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                const float a = 1.0f - t;
                if (a < slideAlpha) slideAlpha = a;
            }
        }
    }

    renderer.SetGlobalAlpha(slideAlpha);
    renderer.DrawMesh(bgMesh, model, Vector3(1.f, 1.f, 1.f), slides[index].tex);

    renderer.SetGlobalAlpha(prevAlpha);
    renderer.setCamera(prevCam);
}
