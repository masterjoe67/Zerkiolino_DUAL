#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include "../petit_fatfs/pff.h"

// Indirizzo base del Gateway VGA (da adattare al tuo setup)
#define VGA_BASE          0x40000000
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

// Registri Dati (Preparano i fili interni del Subsystem)
#define VGA_REG_X         (*(volatile uint32_t*)(VGA_BASE + 0x04)) // Cmd 001
#define VGA_REG_Y         (*(volatile uint32_t*)(VGA_BASE + 0x08)) // Cmd 010
#define VGA_REG_COLOR     (*(volatile uint32_t*)(VGA_BASE + 0x0C)) // Cmd 011
#define VGA_REG_WPAGE     (*(volatile uint32_t*)(VGA_BASE + 0x10)) // Cmd 100
#define VGA_REG_RPAGE     (*(volatile uint32_t*)(VGA_BASE + 0x14)) // Cmd 101
#define VGA_REG_MODE      (*(volatile uint16_t*)(VGA_BASE + 0x1C)) // Cmd 111

#define VGA_MODE_NOMINAL    0x0000  // 640x480 standard
#define VGA_MODE_AUTOINC    0x0001  // Bit 0: Incremento X automatico
#define VGA_MODE_SCALE_H    0x0002  // Bit 1: Scaling 2x Orizzontale
#define VGA_MODE_SCALE_V    0x0004  // Bit 2: Scaling 2x Verticale

// La "Combo" per Doom (320x200 scalato a tutto schermo con auto-inc)
#define VGA_MODE_DOOM       (VGA_MODE_AUTOINC | VGA_MODE_SCALE_H | VGA_MODE_SCALE_V)

// Colori Base RGB565
#define BLACK     0x0000
#define WHITE     0xFFFF
#define RED       0xF800
#define GREEN     0x07E0
#define BLUE      0x001F
#define YELLOW    0xFFE0
#define CYAN      0x07FF
#define MAGENTA   0xF81F
#define GREY      0x8410

//#define LOAD_FONT2
#define LOAD_GLCD

// Registro di Controllo (Trigger Fisico)
// Scrivere qui attiva il segnale 'vga_st_i' nel VHDL per 1 ciclo di clock
#define VGA_STROBE        (*(volatile uint32_t*)(VGA_BASE + 0x18)) 

// Feedback di Stato
// Legge il bit 0 che è collegato a 'fifo_full' (vga_busy_o)
#define VGA_IS_BUSY       ((*(volatile uint32_t*)(VGA_BASE)) & 0x01)

static inline void vga_wait_ready(void) {
    while (VGA_IS_BUSY);
}

// Funzioni base
void vga_set_write_page(uint8_t page);
void vga_set_read_page(uint8_t page);


void vga_clear(uint16_t color);
void vga_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void vga_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void vga_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void vga_put_pixel(uint16_t x, uint16_t y, uint16_t color);
FRESULT vga_load_image_pfs(const char* filename);
int vga_drawChar(unsigned int uniCode, int x, int y, int font);
void vga_set_cursor(int x, int y);
void vga_setTextFont(uint8_t f);
void vga_setTextSize(uint8_t s);
void vga_setTextColor(uint16_t c, uint16_t b);
void vga_Print(const char *str);
size_t vga_write(uint8_t uniCode);



#endif