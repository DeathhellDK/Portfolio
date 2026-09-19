
/**
 * @file      audio.cpp
 * @author    Low JianLin
 * @email     jianlin.low, t.weiliangterril
 * @co-author Tan Wei Liang Terril
 * @date      2025-10-15
 *
 * @brief     Implementation of the Audio module using FMOD
 *
 *  This file contains all low-level audio system logic:
 * - System initialization and shutdown
 * - Sound asset loading/unloading
 * - Voice (channel) pool and playback management
 * - Bus-based volume control (Master/Music/Sfx)
 * - Global sample clock and pause/resume control
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include "Audio/audio.h"
#include <fmod.hpp>
#include <fmod_errors.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "Input/DebugConsole.hpp"

namespace {

    // ---------- helpers ----------
    inline float clampf(float x, float lo, float hi) { return (std::max)(lo, (std::min)(hi, x)); }
    inline float dbToGain(float db) { return std::pow(10.0f, db / 20.0f); }
    inline float gainToDb(float g) { return 20.0f * std::log10((std::max)(g, 1e-8f)); }

    inline void FMOD_CHK(FMOD_RESULT r, const char* where) {
        if (r != FMOD_OK) {
            DebugConsole::Get().Error(std::string("[FMOD] ") + where + " failed: " + FMOD_ErrorString(r));
        }
    }

    // ---------- state ----------
    struct State {
        FMOD::System* sys = nullptr;

        // Channel groups act as our buses
        FMOD::ChannelGroup* master = nullptr;
        FMOD::ChannelGroup* music = nullptr;
        FMOD::ChannelGroup* sfx = nullptr;

        // Filters
        FMOD::DSP* lowPassDSP = nullptr;

        float currentLPFFreq = 22000.0f; // Default "open" frequency
        float targetLPFFreq = 22000.0f;
        float lpfFadeSpeed = 0.0f;       // How much to change per frame

        // Assets
        std::vector<FMOD::Sound*> sounds;

        // Voices (channels) + handle book-keeping like before
        std::vector<FMOD::Channel*> channels;        // slots (size = maxVoices)
        std::vector<uint32_t>       generations;     // generation per slot
        std::vector<uint32_t>       freelist;        // free channel slots

        // Device info
        int deviceSampleRate = 48000;
        int deviceChannels = 2;

        // Global DSP clock (queried from FMOD); we expose it via Audio::GetClock
        std::atomic<uint64_t> lastDSPClock{ 0 };

        // Utility
        FMOD::ChannelGroup* groupForBus(Audio::Bus b) const {
            switch (b) {
            case Audio::Bus::Music: return music;
            case Audio::Bus::Sfx:   return sfx;
            case Audio::Bus::Master:
            default:                return master;
            }
        }
    } S;

    struct Handle { uint32_t index = 0, generation = 0; };
    inline Audio::VoiceHandle pack(const Handle& h) {
        const uint32_t packedIndex = (h.index + 1u) & 0xFFFFu;
        return (Audio::VoiceHandle)((h.generation << 16) | packedIndex);
    }
    inline Handle unpack(Audio::VoiceHandle vh) {
        const uint32_t packedIndex = (uint32_t)(vh & 0xFFFFu);

        if (packedIndex == 0u)
            return { 0xFFFFFFFFu, 0u };

        return { packedIndex - 1u, (uint32_t)((vh >> 16) & 0xFFFFu) };
    }

} // anon

namespace Audio {

    static bool g_initialised = false;

    static bool g_musicPausePending = false;
    static unsigned long long g_musicPauseDspEnd = 0;
    static float g_musicTargetVol = 1.0f; // what music volume should be when unpaused

    // --- Helper functions ---
    static unsigned long long NowDsp()
    {
        unsigned long long dsp = 0, dummy = 0;
        if (S.master) S.master->getDSPClock(&dsp, &dummy);
        return dsp;
    }

    static unsigned long long MsToDsp(unsigned long long ms)
    {
        // Uses existing stored device sample rate: S.deviceSampleRate
        return (ms * (unsigned long long)S.deviceSampleRate) / 1000ull;
    }

    static void ClearMusicFadePoints()
    {
        if (!S.music) return;
        S.music->removeFadePoints(0ull, ~0ull); // clear all fade automation
    }

    /**
     * @brief Initialise the FMOD audio system.
     *
     * Creates the FMOD system, configures buffers, sets up buses (Master/Music/Sfx),
     * and allocates channel slots for voice playback.
     *
     * @param cfg Audio system configuration (sample rate, channels, max voices).
     * @return true if initialisation succeeded, false otherwise.
     */
    bool Initialise(const Settings& cfg) {
        if (g_initialised) return true;

        // 1) System
        FMOD_RESULT r = FMOD::System_Create(&S.sys);
        FMOD_CHK(r, "System_Create");
        if (r != FMOD_OK) return false;

        // Get / set software format to match requested cfg where reasonable
        int samplerate = 0;
        FMOD_SPEAKERMODE sm;
        int maxhdr = 0;
        S.sys->getSoftwareFormat(&samplerate, &sm, &maxhdr);
        if (cfg.deviceSampleRate > 0) samplerate = cfg.deviceSampleRate;
        r = S.sys->setSoftwareFormat(samplerate, sm, maxhdr);
        FMOD_CHK(r, "setSoftwareFormat");

        // Slightly larger DSP buffer helps avoid pops
        // (bufferlength, numbuffers) e.g., 1024 * 4
        S.sys->setDSPBufferSize(1024, 4);

        r = S.sys->init(cfg.maxVoices, FMOD_INIT_NORMAL, nullptr);
        FMOD_CHK(r, "System::init");
        if (r != FMOD_OK) return false;

        S.deviceSampleRate = samplerate;
        S.deviceChannels = (cfg.deviceChannels <= 0 ? 2 : cfg.deviceChannels);

        // 2) Buses
        r = S.sys->getMasterChannelGroup(&S.master); FMOD_CHK(r, "getMasterChannelGroup");
        r = S.sys->createChannelGroup("Music", &S.music); FMOD_CHK(r, "createChannelGroup(Music)");
        r = S.sys->createChannelGroup("Sfx", &S.sfx);   FMOD_CHK(r, "createChannelGroup(Sfx)");
        if (S.master) {
            S.master->addGroup(S.music);
            S.master->addGroup(S.sfx);
        }

        // 3) Low-Pass Filter
        r = S.sys->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &S.lowPassDSP);
        FMOD_CHK(r, "createDSP");

        if (S.lowPassDSP && S.master) {
            // Add to the Master group at index 0
            S.master->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, S.lowPassDSP);
            // Ensure it starts at the "clear" frequency
            S.lowPassDSP->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, 22000.0f);
            S.lowPassDSP->setBypass(false);
        }

        // 4) Voices pool
        S.channels.assign((size_t)cfg.maxVoices, nullptr);
        S.generations.assign((size_t)cfg.maxVoices, 0);
        S.freelist.clear();
        for (uint32_t i = 0; i < (uint32_t)S.channels.size(); ++i)
            S.freelist.push_back((uint32_t)S.channels.size() - 1 - i);

        // Defaults from your config later
        g_initialised = true;
        return true;
    }

    /**
    * @brief Per-frame audio update.
    *
    * Advances FMOD�s internal mixer and DSP graph.
    * Should be called once per frame in the main game loop.
    */
    void Update() {
        if (!g_initialised || !S.sys) return;

        // Let FMOD advance its internal state
        S.sys->update();

        // --- New: LPF Fading Logic ---
        if (std::abs(S.currentLPFFreq - S.targetLPFFreq) > 1.0f) {
            // Delta time (assuming 60fps for simplicity, or pass in actual dt)
            float dt = 1.0f / 60.0f;
            S.currentLPFFreq += S.lpfFadeSpeed * dt;

            // Prevent overshooting
            if ((S.lpfFadeSpeed > 0 && S.currentLPFFreq > S.targetLPFFreq) ||
                (S.lpfFadeSpeed < 0 && S.currentLPFFreq < S.targetLPFFreq)) {
                S.currentLPFFreq = S.targetLPFFreq;
            }

            S.lowPassDSP->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, S.currentLPFFreq);
        }

        // If a fade-to-pause is pending, apply the pause once time has passed
        if (g_musicPausePending && S.music)
        {
            const unsigned long long now = NowDsp();
            if (now >= g_musicPauseDspEnd)
            {
                S.music->setPaused(true);
                ClearMusicFadePoints();

                // Reset volume so future resumes without fade behave
                S.music->setVolume(g_musicTargetVol);

                g_musicPausePending = false;
            }
        }

        // Reclaim finished one-shot voices
        for (uint32_t i = 0; i < (uint32_t)S.channels.size(); ++i) {
            FMOD::Channel* ch = S.channels[i];
            if (!ch) continue;

            bool playing = false;
            FMOD_RESULT r = ch->isPlaying(&playing);

            // If FMOD reports an error or it's no longer playing, free this slot
            if (r != FMOD_OK || !playing) {
                S.channels[i] = nullptr;
                S.freelist.push_back(i);
                S.generations[i] = (S.generations[i] + 1u) & 0xFFFFu;
            }
        }
    }

    /**
    * @brief Shutdown and clean up the FMOD system.
    *
    * Stops all channels, releases sounds, and closes the device.
    * Resets all internal state and deallocates the voice pool.
    */
    void Shutdown() {
        if (!g_initialised) return;

        // Stop every channel
        for (auto* ch : S.channels) if (ch) ch->stop();

        for (auto* s : S.sounds)
            if (s) s->release();
        S.sounds.clear();

        if (S.sfx) { S.sfx->release(); S.sfx = nullptr; }
        if (S.music) { S.music->release(); S.music = nullptr; }

        if (S.sys) {
            S.sys->close();
            S.sys->release();
            S.sys = nullptr;
        }

        S.channels.clear();
        S.generations.clear();
        S.freelist.clear();

        g_initialised = false;
    }

    /**
     * @brief Checks if a specific voice is currently playing.
     * @param vh VoiceHandle to check.
     * @return True if the channel is active and playing.
     */
    bool IsPlaying(VoiceHandle vh) {
        Handle h = unpack(vh);
        if (h.index == 0xFFFFFFFFu || h.index >= S.channels.size()) return false;

        FMOD::Channel* ch = S.channels[h.index];
        if (!ch) return false;

        bool playing = false;
        ch->isPlaying(&playing);
        return playing;
    }

    /**
     * @brief Updates the gain/volume of an active voice.
     * @param vh VoiceHandle of the active sound.
     * @param gain New volume level (0.0 to 1.0).
     */
    void SetGain(VoiceHandle vh, float gain) {
        Handle h = unpack(vh);
        if (h.index == 0xFFFFFFFFu || h.index >= S.channels.size()) return;

        FMOD::Channel* ch = S.channels[h.index];
        if (ch) {
            ch->setVolume(clampf(gain, 0.0f, 10.0f));
        }
    }

    /**
     * @brief Updates the stereo panning of an active voice.
     * @param vh VoiceHandle of the active sound.
     * @param pan New pan value (-1.0 to 1.0).
     */
    void SetPan(VoiceHandle vh, float pan) {
        Handle h = unpack(vh);
        if (h.index == 0xFFFFFFFFu || h.index >= S.channels.size()) return;

        FMOD::Channel* ch = S.channels[h.index];
        if (ch) {
            ch->setPan(clampf(pan, -1.0f, 1.0f));
        }
    }

    /**
    * @brief Load a sound from file into memory.
    *
    * Supports WAV, OGG, MP3 depending on FMOD build.
    *
    * @param path File path to the sound asset.
    * @return SoundID index, or -1 if loading failed.
    */
    SoundID LoadSound(const std::string& path) {
        if (!g_initialised || !S.sys) return -1;

        // For short SFX: FMOD_DEFAULT | FMOD_2D | FMOD_CREATESAMPLE
        // For large music: consider FMOD_CREATESTREAM instead.
        FMOD::Sound* snd = nullptr;
        FMOD_MODE mode = FMOD_DEFAULT | FMOD_2D | FMOD_CREATESAMPLE;
        FMOD_RESULT r = S.sys->createSound(path.c_str(), mode, nullptr, &snd);
        if (r != FMOD_OK || !snd) {
            DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[FMOD] Failed to load: " + path + " : " + FMOD_ErrorString(r), "\n");
            return -1;
        }
        S.sounds.push_back(snd);
        return (SoundID)S.sounds.size() - 1;
    }

    /**
     * @brief Unload a sound from memory.
     *
     * @param sid SoundID to release.
     */
    void UnloadSound(SoundID sid) {
        if (sid < 0 || (size_t)sid >= S.sounds.size()) return;
        if (S.sounds[sid]) { S.sounds[sid]->release(); S.sounds[sid] = nullptr; }
    }

    // Helper to convert our Bus enum to channel group volume
    static FMOD::ChannelGroup* grp(Audio::Bus b) { return S.groupForBus(b); }

    /**
     * @brief Play a sound with optional parameters.
     *
     * Creates a new FMOD channel, sets gain/pan/looping, and starts playback.
     * Supports sample-accurate start scheduling if `desc.startAtSamples` is set.
     *
     * @param sid  SoundID of the loaded sound.
     * @param desc Playback description (gain, pan, loop, bus, etc.).
     * @return VoiceHandle for this playback instance, or 0 if failed.
     */
    VoiceHandle Play(SoundID sid, const PlayDesc& desc) {
        if (sid < 0 || (size_t)sid >= S.sounds.size() || !S.sounds[sid]) return 0;
        if (S.freelist.empty()) return 0;

        uint32_t idx = S.freelist.back(); S.freelist.pop_back();

        FMOD::Channel* ch = nullptr;
        FMOD_RESULT r = S.sys->playSound(S.sounds[sid], grp(desc.bus), true /*start paused*/, &ch);
        if (r != FMOD_OK || !ch) {
            std::string debugMsg = FMOD_ErrorString(r);
            DebugConsole::Get().AddFormattedMessage(LogLevel::Error, "[FMOD] playSound failed: " + debugMsg, "\n");
            //DebugConsole::Get().Error("[FMOD] playSound failed: " + debugMsg);
            S.freelist.push_back(idx);
            return 0;
        }

        // Basic params
        ch->setVolume(clampf(desc.gain, 0.0f, 10.0f));
        ch->setPan(clampf(desc.pan, -1.0f, +1.0f));
        ch->setMode(desc.loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

        // schedule a START, no end:
        if (desc.startAtSamples != 0) {
            unsigned long long parentClock = 0, dummy = 0;
            if (S.master) S.master->getDSPClock(&parentClock, &dummy);

            const unsigned long long startClock =
                (desc.startAtSamples > parentClock) ? desc.startAtSamples : parentClock;

            // older/newer FMOD both support this overload:
            // setDelay(dspclock_start, dspclock_end, stopchannels)
            ch->setDelay(startClock, 0 /*no end*/, false /*don't auto-stop at end*/);
        }

        ch->setPaused(false);

        S.channels[idx] = ch;
        Audio::VoiceHandle vh = pack({ idx, (S.generations[idx] & 0xFFFFu) });
        return vh;
    }

    /**
     * @brief Stop a single playing voice.
     *
     * Applies optional fade-out to prevent pops.
     *
     * @param vh VoiceHandle of the sound to stop.
     * @param fadeOutMs Fade-out duration in milliseconds.
     */
    void Stop(VoiceHandle vh, int fadeOutMs) {
        if (vh == 0) return;
        Handle h = unpack(vh);
        if (h.index == 0xFFFFFFFFu) return;
        if (h.index >= S.channels.size()) return;

        FMOD::Channel* ch = S.channels[h.index];
        if (!ch) return;

        // verify generation: there�s no direct channel-id in FMOD to compare,
        // but we avoid reuse bugs by only freeing the slot once we stop it.
        // If someone called Stop on a stale handle after the slot was reused,
        // you�d need bookkeeping. For simplicity, we assume caller keeps up.

        if (fadeOutMs > 0) {
            unsigned long long dspNow = 0, dummy = 0;
            if (S.master) S.master->getDSPClock(&dspNow, &dummy);
            const unsigned long long dspEnd = dspNow +
                (unsigned long long)((uint64_t)fadeOutMs * (uint64_t)S.deviceSampleRate / 1000ull);

            ch->addFadePoint(dspNow, 1.0f);
            ch->addFadePoint(dspEnd, 0.0f);

            // schedule END only; don't stop channels automatically (we�ll recycle our slot)
            ch->setDelay(0 /*no start change*/, dspEnd /*end time*/, true /*stopchannels*/);
        }
        else {
            ch->stop();
        }

        // recycle slot immediately (FMOD will stop later in fade case)
        S.freelist.push_back(h.index);
        S.generations[h.index] = (S.generations[h.index] + 1) & 0xFFFFu;
        S.channels[h.index] = nullptr;
    }

    /**
     * @brief Stop all active voices.
     *
     * Optionally applies a global fade-out.
     *
     * @param fadeOutMs Fade-out duration in milliseconds.
     */
    void StopAll(int fadeOutMs) {
        if (!g_initialised) return;

        // Hard stop
        if (fadeOutMs <= 0)
        {
            if (S.music) S.music->stop();
            if (S.sfx)   S.sfx->stop();
            // Optionally: S.master->stop(); (usually not needed)
        }
        else
        {
            // Fade both groups, then stop them.
            unsigned long long dspNow = 0, dummy = 0;
            if (S.master) S.master->getDSPClock(&dspNow, &dummy);

            const unsigned long long dspEnd =
                dspNow + (unsigned long long)((uint64_t)fadeOutMs * (uint64_t)S.deviceSampleRate / 1000ull);

            auto fadeAndStopGroup = [&](FMOD::ChannelGroup* g)
                {
                    if (!g) return;
                    g->addFadePoint(dspNow, 1.0f);
                    g->addFadePoint(dspEnd, 0.0f);
                };

            fadeAndStopGroup(S.music);
            fadeAndStopGroup(S.sfx);

            // Quick pragmatic option (no scheduling): just stop immediately after adding fade points
            // so the fade will apply.
            if (S.music) S.music->stop();
            if (S.sfx)   S.sfx->stop();
        }

        // Clear bookkeeping
        for (uint32_t i = 0; i < (uint32_t)S.channels.size(); ++i) {
            if (S.channels[i]) {
                S.channels[i] = nullptr;
                S.freelist.push_back(i);
                S.generations[i] = (S.generations[i] + 1) & 0xFFFFu;
            }
        }
    }

    /**
    * @brief Pause all currently playing channels.
    */
    void PauseAll() {
        if (!g_initialised) return;
        if (S.music) S.music->setPaused(true);
        if (S.sfx)   S.sfx->setPaused(true);
    }

    /**
    * @brief Pause all currently playing bgms.
    */
    void PauseBgm(int fadeMs) {
        if (!g_initialised || !S.music) return;

        bool paused = false;
        S.music->getPaused(&paused);
        if (paused) return;

        // remember what the music bus volume currently is
        S.music->getVolume(&g_musicTargetVol);

        if (fadeMs <= 0)
        {
            S.music->setPaused(true);
            return;
        }

        ClearMusicFadePoints();

        const unsigned long long now = NowDsp();
        const unsigned long long end = now + MsToDsp((unsigned long long)fadeMs);

        // Fade down to 0 (do NOT permanently set volume to 0 here)
        S.music->addFadePoint(now, g_musicTargetVol);
        S.music->addFadePoint(end, 0.0f);

        g_musicPausePending = true;
        g_musicPauseDspEnd = end;
    }

    /**
     * @brief Resume all paused channels.
     */
    void ResumeAll() {
        if (!g_initialised) return;
        if (S.music) S.music->setPaused(false);
        if (S.sfx)   S.sfx->setPaused(false);
    }

    void ResumeBgm(int fadeMs) {
        if (!g_initialised || !S.music) return;

        g_musicPausePending = false;

        ClearMusicFadePoints(); // IMPORTANT

        // Unpause first
        S.music->setPaused(false);

        // Get current volume (could be 0 if you paused recently)
        float curVol = 1.0f;
        S.music->getVolume(&curVol);

        if (fadeMs <= 0)
        {
            S.music->setVolume(g_musicTargetVol);
            return;
        }

        const unsigned long long now = NowDsp();
        const unsigned long long end = now + MsToDsp((unsigned long long)fadeMs);

        // Fade from current -> target (no forced setVolume(0))
        S.music->addFadePoint(now, curVol);
        S.music->addFadePoint(end, g_musicTargetVol);
    }

    /**
     * @brief Set the volume of a given audio bus in decibels.
     *
     * @param bus Target bus (Master, Music, Sfx).
     * @param db  Desired volume in decibels.
     */
    void SetBusDb(Bus bus, float db) {
        float g = clampf(dbToGain(db), 0.0f, 10.0f);
        FMOD::ChannelGroup* cg = S.groupForBus(bus);
        if (cg) cg->setVolume(g);
    }

    /**
     * @brief Get the current volume of a given audio bus.
     *
     * @param bus Target bus (Master, Music, Sfx).
     * @return Volume in decibels.
     */
    float GetBusDb(Bus bus) {
        FMOD::ChannelGroup* cg = S.groupForBus(bus);
        float g = 1.0f;
        if (cg) cg->getVolume(&g);
        return gainToDb(g);
    }

    /**
     * @brief Sets the Low Pass Filter frequency with an optional smooth transition.
     *
     * Adjusts the cutoff frequency of the master LPF. If fadeMs is greater than 0,
     * the frequency will interpolate over time during Update() to avoid audio pops.
     *
     * @param targetFreq The desired cutoff frequency in Hz (10.0 to 22000.0).
     * @param fadeMs     Duration of the transition in milliseconds.
     */
    void SetLowPass(float targetFreq, int FadeMs) {
        if (!S.lowPassDSP)
            return;
        S.targetLPFFreq = clampf(targetFreq, 10.0f, 22000.0f);

        if (FadeMs <= 0) {
            S.currentLPFFreq = S.targetLPFFreq;
            S.lowPassDSP->setParameterFloat(FMOD_DSP_ITLOWPASS_CUTOFF, S.currentLPFFreq);
        }
        else {
            float diff = S.targetLPFFreq - S.currentLPFFreq;
            S.lpfFadeSpeed = diff / (FadeMs / 1000.0f);
        }

    }

    /**
     * @brief Retrieve the global audio clock in samples.
     *
     * Queries FMOD�s DSP clock, stores it, and returns it as a 64-bit counter.
     * This value can be used for sample-accurate scheduling.
     *
     * @return Current DSP clock in samples.
     */
    uint64_t GetClock() {
        if (!S.master) return S.lastDSPClock.load(std::memory_order_relaxed);
        unsigned long long dspNow = 0, dummy = 0;
        S.master->getDSPClock(&dspNow, &dummy);
        S.lastDSPClock.store((uint64_t)dspNow, std::memory_order_relaxed);
        return (uint64_t)dspNow;
    }

} // end of namespace Audio
