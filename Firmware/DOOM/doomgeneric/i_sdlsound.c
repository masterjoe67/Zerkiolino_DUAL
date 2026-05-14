#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#include "i_sound.h"
#include "i_sound_neorv32.h"
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

// Numero di campioni da mixare per ogni chiamata (circa 1/35 di secondo)
#define MIXBUFFER_SIZE 320
#define MIXBUFFER_SIZE 320 

void I_SDL_UpdateSound(void) {
    if (!sound_initialized) return;

    // Recuperiamo i canali audio dal motore di Doom (variabile standard)
    extern channel_t channels[8]; 

    for (int i = 0; i < MIXBUFFER_SIZE; i++) {
        int mixed_sample = 0;
        int active_count = 0;

        // 1. MIXER SOFTWARE: Sommiamo i campioni dagli 8 canali
        for (int c = 0; c < 8; c++) {
            if (channels[c].sfxinfo && channels[c].step) {
                // Prendiamo il byte dal WAD in SDRAM
                uint8_t* sound_data = (uint8_t*)channels[c].data;
                uint8_t sample = sound_data[channels[c].position >> 16];
                
                // Lo sommiamo (centrandolo sullo zero per il mixaggio)
                mixed_sample += (int)sample - 128;
                active_count++;

                // Avanziamo nella lettura del suono (logica originale di Doom)
                channels[c].position += channels[c].step;

                // Se il suono è finito, liberiamo il canale
                if ((channels[c].position >> 16) >= channels[c].sfxinfo->length) {
                    channels[c].sfxinfo = NULL;
                }
            }
        }

        // 2. NORMALIZZAZIONE: Riportiamo il valore nel range 0-255
        if (active_count > 0) {
            mixed_sample = (mixed_sample / active_count) + 128;
        } else {
            mixed_sample = 128; // Silenzio
        }

        // 3. INVIO ALL'FPGA
        // Aspetta che la FIFO abbia spazio
        while (*(volatile uint32_t*)0x80000000 & 0x1); 

        // Invia il campione mixato al DAC (MCP4902)
        uint16_t out_val = 0x3000 | ((uint8_t)mixed_sample << 4);
        *(volatile uint16_t*)0x80000020 = out_val;
    }
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
// Pulisci i registri audio se necessario
    /* *(volatile uint16_t*)AUDIO_REG_ADDR = 0x3800; // Valore centrale (silenzio)
    
    printf("Tiny_SoundBlaster: Hardware initialized at 11025Hz\n");
    sound_initialized = true;
    return true;*/
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