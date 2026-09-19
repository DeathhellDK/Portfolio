#pragma once
/**
 * @file      audio.h
 * @author    Low JianLin
 * @email     jianlin.low
 * @date      2025-10-15
 *
 * @brief     Public audio API for the game engine.
 *
 * This module exposes:
 *   - System init/shutdown
 *   - Loading/unloading decoded sounds
 *   - Playing/stopping sounds (with gain/pan/loop/etc.)
 *   - Bus (Master/Music/Sfx) volume control
 *   - Global sample-accurate timing
 *   - Pause/resume all audio
 *
 * Internally, the implementation uses FMOD to manage voices,
 * channel groups (buses), and sample-accurate scheduling.
 *
 * @version 1.0
 * @copyright Copyright (C) 2025
 * DigiPen Institute of Technology. Reproduction or disclosure of this file or
 * its contents without the prior written consent of DigiPen Institute of
 * Technology is prohibited.
 */
#include <cstdint>
#include <string>

 /* Components needed for the audio system :
 1) Audio Device + Callback
 2) Global Sample Clock
 3) Command Queue
 4) Sound Loader/Decoder
 5) Sound Asset
 6) Voice/Voice Pool
 7) Mixer (per-buffer & per-voice)
 8) Buses
 9) Fader
 10) Public API (Functions called by game)
 */

namespace Audio {
	using SoundID = int;								// ID of sound loaded into memory
	using VoiceHandle = std::uint32_t;					// Specific handle to an instance of active playback of a SoundID

	enum class Bus { Master, Music, Sfx };				// Logical group of voices with shared volume control

	struct Settings {									// Configuration to pass to Initialise()
		int deviceSampleRate = 48000;					// 48kHz
		int deviceChannels = 2;							// 2 for stereo, 1 for mono
		int maxVoices = 64;								// Max number of simultaneous sounds allowed
	};

	struct PlayDesc {									// Play Description, used to customise playback when calling Play()
		float gain = 1.0f;								// Volume multiplier. 1.0 = normal, 0.5 = half volume
		float pan = 0.0f;								// left/right balance. 0.0 = center, -1.0 = full left, +1.0 = full right
		bool loop = false;								// loop, if true, sound restarts after reaching the end
		uint64_t startAtSamples = 0;					// used for Sample Accuracy. Absolute sample index at which to start
		// - If 0, play now. (engine will replace 0 with current global sample clock)
		// - If not 0, sound is scheduled to start at exact sample position
		Bus bus = Bus::Sfx;
	};

	// lifecycle
	/**
	 * @brief Initializes the audio system.
	 * @param cfg Configuration settings.
	 * @return True if initialization succeeded, false otherwise.
	 */
	bool Initialise(const Settings& cfg);				// Start the Audio system, returns true on success, false on failure
	
	/** @brief Updates the audio system logic (called per frame). */
	void Update();

	/** @brief Shuts down the audio system and releases resources. */
	void Shutdown();									// Cleanly stop audio and release resources
	// - Components: Audio Device, State

// update
	/**
	 * @brief Checks if a specific voice is currently playing.
	 * @param vh The voice handle to check.
	 * @return True if playing.
	 */
	bool IsPlaying(VoiceHandle vh);
	
	/**
	 * @brief Sets the gain (volume) for a specific voice.
	 * @param vh The voice handle.
	 * @param gain Gain multiplier.
	 */
	void SetGain(VoiceHandle vh, float gain);
	
	/**
	 * @brief Sets the pan (stereo balance) for a specific voice.
	 * @param vh The voice handle.
	 * @param pan Pan value (-1.0 to 1.0).
	 */
	void SetPan(VoiceHandle vh, float pan);

	// assets
	/**
	 * @brief Loads a sound file into memory.
	 * @param path Path to the audio file.
	 * @return A unique SoundID.
	 */
	SoundID LoadSound(const std::string& path);			// Load & decode a file(WAV/OGG/MP3) into RAM as PCM floats
	// - Components: Decoder, Sound Asset Store
	
	/**
	 * @brief Unloads a previously loaded sound from memory.
	 * @param sid The SoundID to unload.
	 */
	void	UnloadSound(SoundID sid);					// Free a previously loaded sound's memory
	// - Components: Sound Asset Store

// playback
	/**
	 * @brief Plays a loaded sound with specified settings.
	 * @param sid The SoundID to play.
	 * @param desc Playback description (gain, pan, loop, etc.).
	 * @return A VoiceHandle for the active playback instance.
	 */
	VoiceHandle Play(SoundID sid, const PlayDesc& desc = {});	// Request to play a sound with settings (gain, pan etc)
	// - Components: Command Queue, Audio Callback, Mixer
	
	/**
	 * @brief Stops a specific playing voice with an optional fade-out.
	 * @param vh The voice handle to stop.
	 * @param fadeOutMs Fade-out duration in milliseconds.
	 */
	void		Stop(VoiceHandle vh, int fadeOutMs = 20);		// Request to stop a specific playing voice, with short fade to avoid clicks
	// - Components: Command Queue, Audio Callback
	
	/**
	 * @brief Stops all active voices with an optional fade-out.
	 * @param fadeOutMs Fade-out duration in milliseconds.
	 */
	void		StopAll(int fadeOutMs = 20);					// Fade out every actve voice
	// - Components: Command Queue, Audio Callback

// buses
	/**
	 * @brief Sets the volume for a specific bus in decibels.
	 * @param bus The bus to modify.
	 * @param db Volume in dB.
	 */
	void	SetBusDb(Bus bus, float db);						// Set per-bus volume (in dB) for Master/Music/Sfx
	// Components: Command Queue, Audio Callback / Mixer, Buses		
	
	/**
	 * @brief Gets the current volume of a specific bus in decibels.
	 * @param bus The bus to query.
	 * @return Volume in dB.
	 */
	float	GetBusDb(Bus bus);									// Reads the current bus volume (in dB)
	// - Components: Buses

// filters
	// cutoff: 10.0 to 22000.0 Hz
	/**
	 * @brief Applies a low-pass filter to the master bus.
	 * @param targetFreq Cutoff frequency in Hz.
	 * @param FadeMs Fade transition duration in milliseconds.
	 */
	void SetLowPass(float targetFreq, int FadeMs);
	// - Components: Buses

// clock
	/**
	 * @brief Gets the global sample clock.
	 * @return Absolute samples since audio system started.
	 */
	uint64_t GetClock();										// Read the Global Sample Clock (absolute samples since start)
	// used for sample-accurate playback
	// - Components: Global Sample Clock

	/** @brief Pauses all currently playing channels. */
	void PauseAll();   // Pause all currently playing channels
	
	/**
	 * @brief Pauses background music with a fade out.
	 * @param fadeMs Fade-out duration in milliseconds.
	 */
	void PauseBgm(int fadeMs = 200);
	
	/** @brief Resumes all paused channels. */
	void ResumeAll();  // Resume all paused channels
	
	/**
	 * @brief Resumes background music with a fade in.
	 * @param fadeMs Fade-in duration in milliseconds.
	 */
	void ResumeBgm(int fadeMs = 200);
}