#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#include "i_sound.h"
#include "doomtype.h"

// --- STUB AUDIO PER NEORV32 ---
// Queste funzioni sostituiscono il driver SDL originale per evitare dipendenze esterne.

static boolean sound_initialized = false;

void I_SDL_UpdateSoundParams(int handle, int vol, int sep) {
    // Non implementato
}

int I_SDL_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep) {
    return channel; // Simula l'avvio del suono restituendo il canale
}

void I_SDL_StopSound(int handle) {
    // Non implementato
}

boolean I_SDL_SoundIsPlaying(int handle) {
    return false; // I suoni non finiscono mai perché non iniziano mai
}

void I_SDL_UpdateSound(void) {
    // Non implementato
}

void I_SDL_ShutdownSound(void) {
    sound_initialized = false;
}

int I_SDL_GetSfxLumpNum(sfxinfo_t *sfx) {
    return 0; // Restituisce un lump fittizio
}

void I_SDL_PrecacheSounds(sfxinfo_t *sounds, int num_sounds) {
    // No-op: non carichiamo i suoni in RAM per risparmiare spazio sulla SDRAM
}

boolean I_SDL_InitSound(boolean _use_sfx_prefix) {
    sound_initialized = true;
    return true; 
}

// --- MODULO SONORO ---
// Manteniamo la struttura per compatibilità con il linker di doomgeneric

static snddevice_t sound_sdl_devices[] = {
    SNDDEVICE_SB
};

sound_module_t DG_sound_module = {
    sound_sdl_devices,
    1,
    I_SDL_InitSound,
    I_SDL_ShutdownSound,
    I_SDL_GetSfxLumpNum,
    I_SDL_UpdateSound,
    I_SDL_UpdateSoundParams,
    I_SDL_StartSound,
    I_SDL_StopSound,
    I_SDL_SoundIsPlaying,
    I_SDL_PrecacheSounds,
};