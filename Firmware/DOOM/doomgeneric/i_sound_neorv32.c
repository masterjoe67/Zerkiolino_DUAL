#include <neorv32.h>
#include <stdio.h>
#include "config.h"
#include "i_sound_neorv32.h"
#include "i_sound.h"
#include "s_sound.h"
#include "doomtype.h"
#include "w_wad.h"
#include "z_zone.h"

// 2. Variabili di Stato
static boolean sound_initialized = false;
static snddevice_t sound_sdl_devices[] = { SNDDEVICE_SB };

// 3. Prototipi (Diciamo al compilatore che queste funzioni esistono dopo)

void    I_SDL_ShutdownSound(void);
int     I_SDL_GetSfxLumpNum(sfxinfo_t *sfx);

void    I_SDL_UpdateSoundParams(int handle, int vol, int sep);
int     I_SDL_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep);
void    I_SDL_StopSound(int handle);
boolean I_SDL_SoundIsPlaying(int handle);
void    I_SDL_PrecacheSounds(sfxinfo_t *sounds, int num_sounds);

typedef struct
{
    sfxinfo_t *sfxinfo;
    mobj_t    *origin;
    int        handle;    
} neorv32_channel_t;

extern neorv32_channel_t *channels;
extern int snd_channels;

// 4. La Struttura (Ora il compilatore conosce i nomi sopra!)
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

typedef struct {
    sfxinfo_t* sfxinfo;
    uint32_t   position; // Fixed point 16.16
    uint32_t   step;     // Incremento (pitch)
    int        active;   // 1 se sta suonando
} local_channel_t;

// Creiamo 8 canali locali
static local_channel_t my_channels[8];

sound_module_t* sound_module = &DG_sound_module;

// 5. Implementazione delle Funzioni
boolean I_SDL_InitSound(boolean _use_sfx_prefix) {
    AUDIO_REG_ADDR = 0x3800; // Silenzio (metà scala)
    sound_initialized = true;
    return true; 
}


void I_SDL_UpdateSound2(void) {
    if (!sound_initialized) return;

    // Continua a mixare e scrivere finché la FIFO hardware non è FULL
    // STATUS_REG_ADDR & 0x1 deve essere '1' quando la FIFO è piena [cite: 33, 81]
    while (!(STATUS_REG_ADDR & 0x1)) { 
        
        int mixed_val = 0;
        int active_count = 0;

        for (int c = 0; c < 8; c++) {
            if (my_channels[c].active && my_channels[c].sfxinfo) {
                // Recupero dati tramite il lumpnum di Doom [cite: 58, 80]
                uint8_t *data = (uint8_t *)W_CacheLumpNum(my_channels[c].sfxinfo->lumpnum, 101);
                
                if (data) {
                    uint32_t pos = (my_channels[c].position >> 16) + 8;
                    mixed_val += (int)data[pos] - 128;
                    active_count++;

                    my_channels[c].position += my_channels[c].step;

                    // Controllo fine campione [cite: 62, 63]
                    if ((my_channels[c].position >> 16) >= (W_LumpLength(my_channels[c].sfxinfo->lumpnum) - 8)) {
                        my_channels[c].active = 0;
                    }
                }
            }
        }

        // Se non ci sono suoni attivi, scrivi il valore di riposo (128) per tenere la FIFO piena
        if (active_count > 0) {
            mixed_val = (mixed_val / active_count) + 128;
        } else {
            mixed_val = 128;
        }

        int final_sample = (mixed_val / active_count) + 128;
        if (final_sample > 255) final_sample = 255;
        if (final_sample < 0) final_sample = 0;
        AUDIO_REG_ADDR = 0x3000 | ((uint16_t)final_sample << 4);

       
        
        // Se non ci sono suoni attivi in nessun canale, esci dal loop per non sprecare CPU
        if (active_count == 0) break;
    }
}

void I_SDL_UpdateSound(void) {
    if (!sound_initialized) return;

    // Strutture temporanee per evitare di chiamare funzioni Doom nel loop critico
    uint8_t *chan_data_ptr[8];
    uint32_t chan_end_pos[8];
    int active_channels[8];
    int num_active = 0;

    // --- STAGE 1: PRE-FETCH (Fuori dal loop dei campioni) ---
    for (int c = 0; c < 8; c++) {
        if (my_channels[c].active && my_channels[c].sfxinfo) {
            // Chiamiamo le funzioni pesanti UNA SOLA VOLTA per frame
            chan_data_ptr[c] = (uint8_t *)W_CacheLumpNum(my_channels[c].sfxinfo->lumpnum, 101);
            chan_end_pos[c] = W_LumpLength(my_channels[c].sfxinfo->lumpnum) - 8;
            active_channels[num_active++] = c;
        }
    }

    if (num_active == 0) return;

    // --- STAGE 2: MIXING LOOP (Solo calcoli matematici e accesso bus) ---
    int max_samples = 512; 
    int sent = 0;

    while (!(STATUS_REG_ADDR & 0x1) && (sent < max_samples)) { 
        int mixed_val = 0;
        int current_count = 0;

        for (int i = 0; i < num_active; i++) {
            int c = active_channels[i];
            
            // Accesso diretto ai puntatori pre-calcolati
            uint32_t pos = (my_channels[c].position >> 16);
            
            if (pos < chan_end_pos[c]) {
                mixed_val += (int)chan_data_ptr[c][pos + 8] - 128;
                current_count++;
                my_channels[c].position += my_channels[c].step;
            } else {
                my_channels[c].active = 0;
                // Nota: Non rimuoviamo dal active_channels qui per non sporcare il loop, 
                // i canali spenti verranno saltati al prossimo frame o nel check pos.
            }
        }

        if (current_count == 0) break;

        // Clipping e normalizzazione veloci
        mixed_val = (mixed_val / current_count) + 128;
        if (mixed_val > 255) mixed_val = 255;
        else if (mixed_val < 0) mixed_val = 0;

        // Scrittura bus a 32-bit (Comando 0x8 sulla FPGA)
        AUDIO_REG_ADDR = 0x3000 | ((uint16_t)mixed_val << 4);
        
        sent++;
    }
}

int I_SDL_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep) {
    // 1. Salviamo il riferimento al suono
    my_channels[channel].sfxinfo = sfxinfo;
    
    // 2. Resettiamo la posizione all'inizio del campione
    my_channels[channel].position = 0;
    
    // 3. Impostiamo lo step (velocità). 
    // Doom usa 11025Hz, quindi se il tuo hardware va a 11025Hz, lo step è 1.0 (1 << 16)
    my_channels[channel].step = (1 << 16); 
    
    // 4. Attiviamo il canale
    my_channels[channel].active = 1;

    return channel; // Restituiamo il numero del canale come handle
}

// Tutte le altre funzioni (Shutdown, Stop, etc.) devono essere qui sotto, anche vuote:
void I_SDL_ShutdownSound(void) {}
int  I_SDL_GetSfxLumpNum(sfxinfo_t *sfx) { return 0; }
void I_SDL_UpdateSoundParams(int handle, int vol, int sep) {}
void I_SDL_StopSound(int handle) {}
boolean I_SDL_SoundIsPlaying(int handle) { return false; }
void I_SDL_PrecacheSounds(sfxinfo_t *sounds, int num_sounds) {}

//sound_module_t* sound_module; // Diciamo al compilatore che esiste questa variabile nel motore
//sound_module_t DG_sound_module; // La tua struttura nel file audio

//sound_module_t* sound_module = NULL;

// Funzione helper per inviare il singolo campione
static inline void I_WriteSample(uint8_t sample) {
    // Aspetta che la FIFO FPGA abbia spazio (Busy bit 0)
    while (STATUS_REG_ADDR & 0x1);
    
    // Converti 8-bit in 12-bit e aggiungi config MCP4902 (0x3000)
    uint16_t out_val = 0x3000 | ((uint16_t)sample << 4);
    AUDIO_REG_ADDR = out_val;
}

bool I_Z_InitSound(bool _use_sfx_prefix) {
// Pulisci i registri audio se necessario
    AUDIO_REG_ADDR = 0x3800; // Valore centrale (silenzio)
    
    printf("Tiny_SoundBlaster: Hardware initialized at 11025Hz\n");
// Colleghiamo il puntatore globale del motore alla nostra struttura
    sound_module = &DG_sound_module;

    // Reset hardware (il tuo solito comando per il DAC)
    AUDIO_REG_ADDR = 0x3800; 

    sound_initialized = true;
    return true;
}