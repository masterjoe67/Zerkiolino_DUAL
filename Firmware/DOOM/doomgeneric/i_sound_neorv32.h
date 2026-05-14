#ifndef I_SOUND_NEORV32_H
#define I_SOUND_NEORV32_H

#include <stdint.h>
#include <stdbool.h>
#include "doomtype.h"

#define STATUS_REG_ADDR (*(volatile uint32_t*)(0x40000000)) 
#define AUDIO_REG_ADDR  (*(volatile uint16_t*)(0x40000020))

boolean I_SDL_InitSound(boolean _use_sfx_prefix);
void    I_SDL_UpdateSound(void);


#endif