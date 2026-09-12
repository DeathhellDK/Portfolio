/* Start Header ************************************************************************/
/*!
\file		Audio.cpp
\author		Jia Hao
\date		March, 20, 2025
\brief

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "Audio.h"
#include <stdio.h>
#include "AEEngine.h"
#include "options.h"
// Define global audio variables
static AEAudio MenuMusic, BattleMusic, BossMusic, CreditMusic;
static AEAudio Weapon1ShootingSound, Weapon2ShootingSound, Weapon3ShootingSound;
static AEAudio ShootingHITSound, ClickAudio, PlayerHITSound;
static AEAudio  LevelUpSound;
static AEAudioGroup MusicGroup, SFXGroup;

// Flags for music states
static s32 isMenuMusicPlaying = 0;
static s32 isGameMusicPlaying = 0;


void Audio_Init() {
    MusicGroup = AEAudioCreateGroup();
    SFXGroup = AEAudioCreateGroup();

    // Load audio files
    MenuMusic = AEAudioLoadMusic("./Assets/Music/Menu_music.wav");
    //CreditMusic = AEAudioLoadMusic("./Assets/Music/Credit.wav");
    BattleMusic = AEAudioLoadMusic("./Assets/Music/Battle_music.wav"); // In-game music
    //BossMusic = AEAudioLoadMusic("./Assets/Music/Boss_music.wav");
    // Assign unique shooting sounds per weapon
    Weapon1ShootingSound = AEAudioLoadSound("./Assets/SFX/Weapon1_Shoot.mp3");
    Weapon2ShootingSound = AEAudioLoadSound("./Assets/SFX/Gun_laser.wav");
    Weapon3ShootingSound = AEAudioLoadSound("./Assets/SFX/Gun_pistol.wav");

    // Hit and click sounds
    ShootingHITSound = AEAudioLoadSound("./Assets/SFX/ShootingHIT_01.wav");
    ClickAudio = AEAudioLoadSound("./Assets/SFX/UI_button_click_glass.wav");
    LevelUpSound = AEAudioLoadSound("./Assets/SFX/LevelUP.wav");

    // Sound for when an enemy touches the player
    PlayerHITSound = AEAudioLoadSound("./Assets/SFX/Hitplayer_sound_01.wav");
}

// Define function pointer type
//typedef void (*SoundFunction)(void);

// Individual sound functions
void PlayMenuMusic() { 
    if (!isMenuMusicPlaying) { 
        AEAudioStopGroup(MusicGroup);
        AEAudioPlay(MenuMusic, MusicGroup, 0.5f, 1.0f, -1); 
        isMenuMusicPlaying = 1; 
        isGameMusicPlaying = 0; // Reset game music flag when going back to the menu
        //printf("Sound playing!!\n");
    } 
}

void PlayBackgroundMusic() {
    if (!isGameMusicPlaying) {
        AEAudioStopGroup(MusicGroup); // Stop in-game music before playing menu music
        AEAudioPlay(BattleMusic, MusicGroup, 0.5f, 1.0f, -1);
        isGameMusicPlaying = 1;
        isMenuMusicPlaying = 0;
    }
}
// Weapon-specific shooting sounds


void PlayWeapon1ShootingAudio() {
    AEAudioPlay(Weapon1ShootingSound, SFXGroup, 0.5f, 1.0f, 0);
}
void PlayWeapon2ShootingAudio() {
    AEAudioPlay(Weapon2ShootingSound, SFXGroup, 0.5f, 1.0f, 0);
}
void PlayWeapon3ShootingAudio() {
    AEAudioPlay(Weapon3ShootingSound, SFXGroup, 0.5f, 1.0f, 0);
}

void PlayShootingHitAudio() { 
    AEAudioPlay(ShootingHITSound, SFXGroup, 0.5f, 1.0f, 0);
}
void PlayClickAudio() { 
    AEAudioPlay(ClickAudio, SFXGroup, 0.5f, 0.5f, 0);
}
// Plays when an enemy touches the player
void PlayPlayerHITAudio() {
    AEAudioPlay(PlayerHITSound, SFXGroup, 0.5f, 1.0f, 0);
}
void PlayLevelUpAudio() {
    AEAudioPlay(LevelUpSound, SFXGroup, 0.5f, 1.0f, 0);
}


// Function pointer table
/*static SoundFunction soundFunctions[SOUND_COUNT] = {
    PlayMenuMusic,         // MENU
    PlayBackgroundMusic,
    PlayWeapon1ShootingAudio, // WEAPON1_SHOOT
    PlayWeapon2ShootingAudio, // WEAPON2_SHOOT
    PlayWeapon3ShootingAudio, // WEAPON3_SHOOT
    PlayShootingHitAudio,  // SHOOTING_HIT
    PlayClickAudio,        // CLICK
    PlayPlayerHITAudio     // PLAYER_HIT
};*/

void PlayGameSound(soundtypes sound) {
    switch (sound) {
    case MENU: PlayMenuMusic(); break;
    case BATTLE: PlayBackgroundMusic(); break;
    case WEAPON1_SHOOT: PlayWeapon1ShootingAudio(); break;
    case WEAPON2_SHOOT: PlayWeapon2ShootingAudio(); break;
    case WEAPON3_SHOOT: PlayWeapon3ShootingAudio(); break;
    case SHOOTING_HIT: PlayShootingHitAudio(); break;
    case CLICK: PlayClickAudio(); break;
    case PLAYER_HIT: PlayPlayerHITAudio(); break;
    case LEVELUP: PlayLevelUpAudio(); break;
    default: break;
    }
}


void UpdateAudioVolumes() {
    AEAudioSetGroupVolume(MusicGroup, GetMusicVolume() / 100.0f);
    AEAudioSetGroupVolume(SFXGroup, GetSoundVolume() / 100.0f);
}




// Cleanup
void Audio_Shutdown() {
    AEAudioUnloadAudio(MenuMusic);
    AEAudioUnloadAudio(BattleMusic);
    AEAudioUnloadAudio(Weapon1ShootingSound);
    AEAudioUnloadAudio(Weapon2ShootingSound);
    AEAudioUnloadAudio(Weapon3ShootingSound);
    AEAudioUnloadAudio(ShootingHITSound);
    AEAudioUnloadAudio(ClickAudio);
    AEAudioUnloadAudio(PlayerHITSound);
    AEAudioUnloadAudio(LevelUpSound);

    AEAudioUnloadAudioGroup(MusicGroup);
    AEAudioUnloadAudioGroup(SFXGroup);
}