#ifndef DOOMGENERIC_NEORV32_H
#define DOOMGENERIC_NEORV32_H

#include <stdint.h>
#include "doomgeneric.h"

// --- CONFIGURAZIONE HARDWARE ---
#define DOOM_W_WIDTH  320
#define DOOM_W_HEIGHT 200
#define WAD_SDRAM_ADDR 0x91000000 // Indirizzo in SDRAM 1 dove carichi il WAD

// --- VARIABILI ESTERNE ---
// Dichiariamo qui DG_ScreenBuffer così ogni file che include questo header lo vede
extern pixel_t* DG_ScreenBuffer;

// La palette che useremo per convertire da 8-bit a RGB565
//extern const uint16_t doom_palette_rgb565[256];



// --- PROTOTIPI DOOMGENERIC ---
void DG_Init(void);
void DG_DrawFrame(void);
void DG_SleepMs(uint32_t ms);
uint32_t DG_GetTicksMs(void);
int DG_GetKey(int* outKey, unsigned char* outIsDown) ;

void vga_blit_to_subsystem(uint8_t* source, uint32_t size);
void DG_DrawFrame();
int mkdir(const char *pathname, int mode); // Per compatibilità con le funzioni di file system usate da Doom

#endif // DOOMGENERIC_NEORV32_H