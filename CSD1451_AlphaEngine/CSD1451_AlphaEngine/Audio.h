/* Start Header ************************************************************************/
/*!
\file		Audio.h
\author		Jia Hao
\date		March, 20, 2025
\brief      Declares the interface for managing game audio, including music and sound effects.

This header defines the soundtypes enum for various sound events (menu music, shooting, hits, etc.) 
and declares functions for initializing, playing, stopping, and shutting down audio in the game. 
It also provides a unified way to trigger sounds based on game events using PlayGameSound

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#ifndef AUDIO_H
#define AUDIO_H

#include "AEAudio.h"
// Enum for different sound types
typedef enum {
    MENU,
    BATTLE,
    WEAPON1_SHOOT,
    WEAPON2_SHOOT,
    WEAPON3_SHOOT,
    SHOOTING_HIT,
    CLICK,
    PLAYER_HIT,  // Plays when an enemy touches the player
    LEVELUP,
    SOUND_COUNT
} soundtypes;

// Function declarations
//initialize audio
void Audio_Init(void);
//update volume
void UpdateAudioVolumes();
//free audio
void Audio_Shutdown(void);
//choose which sound/music to play
void PlayGameSound(soundtypes sound);


#endif // AUDIO_H