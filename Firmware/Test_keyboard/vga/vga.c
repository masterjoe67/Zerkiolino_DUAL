#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include "vga.h"
#include "neorv32.h"
#include "../petit_fatfs/pff.h"
#include "fonts/glcdfont.h"

void vga_put_pixel(uint16_t x, uint16_t y, uint16_t color) {
    // Il subsystem ora si aspetta 640x480 reali
    while (VGA_IS_BUSY);
    VGA_REG_X = x;
    VGA_REG_Y = y;
    VGA_REG_COLOR = color; // Invia i 16 bit colore in un colpo solo
    
    
    VGA_STROBE = 1; // Lo strobe fa scattare il mapping Bank0/Bank1 nel VHDL
}



void vga_set_write_page(uint8_t page) {
    VGA_REG_WPAGE = (uint16_t)page;
}

void vga_set_read_page(uint8_t page) {
    VGA_REG_RPAGE = (uint16_t)page;
}

void vga_clear(uint16_t color) {
    vga_fillRect(0, 0, 640, 480, color); 
}

void vga_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    uint16_t x_end = x + w;
    uint16_t y_end = y + h;

    // 1. Carichiamo il colore nel registro una volta sola per tutto il rettangolo
    // Questo risparmia (w * h - 1) scritture sul bus!
    VGA_REG_COLOR = color; 
    VGA_REG_X = x;
    //VGA_REG_Y = y;
    VGA_REG_MODE = 0x0001;
    for (uint16_t i = y; i < y_end; i++) {
        // 2. Impostiamo la coordinata Y una volta per riga
        VGA_REG_Y = i; 
        VGA_REG_X = x;
        for (uint16_t j = x; j < x_end; j++) {
            // 3. Aspettiamo che ci sia posto nella FIFO
            // Bit 0 = (fifo_full or pending_pixel_wr)
            //while(VGA_IS_BUSY); 

            // 4. Aggiorniamo X
            //VGA_REG_X = j;

            // 5. TRIGGER: Diamo l'ordine di sparo.
            // Il VHDL prenderà l'X appena scritto e i reg_y/reg_color salvati.
            VGA_STROBE = 1; 
        }
    }
    VGA_REG_MODE = 0x0000;
}

void vga_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (w < 0) { x += w; w = -w; }
    
    // Imposta Y una sola volta per tutta la linea
    while(VGA_IS_BUSY); 
    VGA_REG_Y = y;

    VGA_REG_COLOR = color;

    for (int16_t i = 0; i < w; i++) {
        int16_t curX = x + i;
        if (curX < 0 || curX >= 640) continue; // Clipping base

        while(VGA_IS_BUSY); 
        VGA_REG_X = curX;
        
        VGA_STROBE = 1;
    }
}

void vga_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (h <= 0) return;
    if (x < 0 || x >= 640) return; // Clipping X

    // Imposta X una sola volta per tutta la linea verticale
    while(VGA_IS_BUSY); 
    VGA_REG_X = x;
    VGA_REG_COLOR = color;

    for (int16_t i = 0; i < h; i++) {
        int16_t curY = y + i;
        if (curY < 0 || curY >= 480) continue; // Clipping Y

        // Attendiamo che la SDRAM sia pronta
        while(VGA_IS_BUSY); 
        
        // Aggiorniamo solo la Y
        VGA_REG_Y = curY;
        
        // Scrittura pixel (trigger su DATA_H)
        VGA_STROBE = 1;
    }
}


FRESULT vga_load_image_pfs(const char* filename) {
    FATFS fs;
    FRESULT res;
    // Cambiamo WORD in UINT per compatibilità con il prototipo di pf_read
    UINT br;             
    uint8_t buffer[512]; 
    uint16_t x = 0, y = 0;

    // Monta e Apri (ormai lo sai, PetitFS è spartano)
    res = pf_mount(&fs);
    if (res != FR_OK) return res;

    res = pf_open(filename);
    if (res != FR_OK) return res;

    // Ciclo di lettura
    while (y < 480) {
        // Ora il puntatore &br è del tipo corretto (UINT *)
        res = pf_read(buffer, 512, &br);
        
        if (res != FR_OK || br == 0) break;

        for (UINT i = 0; i < br; i += 2) {
            // Composizione colore (Racing mode)
            uint16_t color = ((uint16_t)buffer[i+1] << 8) | buffer[i];
            
            vga_put_pixel(x, y, color);

            x++;
            if (x >= 640) {
                x = 0;
                y++;
                if (y >= 480) break;
            }
        }
    }

    return res;
}

#define PSRAM_FONT_BASE 0x90000000
#define FONT_SIZE 640 // 128 caratteri * 5 byte

uint16_t textcolor, textbgcolor, fontsloaded, addr_row, addr_col;
int16_t  cursor_x, cursor_y, win_xe, win_ye, padX;
uint8_t  textfont, textsize, textdatum, rotation;
bool  textwrap = true;

FRESULT load_font_to_psram(void) {
    FRESULT res;
    UINT br;
    uint8_t buffer[32]; 
    uint32_t current_ptr = 0x90000000;
    uint16_t bytes_to_read = 640;
    uint32_t total_written = 0;

    neorv32_uart0_printf("SD: Apertura FONT5X7.BIN... ");
    res = pf_open("FONT5X7.BIN");
    
    if (res != FR_OK) {
        neorv32_uart0_printf("ERRORE! Code: %d\n", (uint32_t)res);
        return res;
    }
    neorv32_uart0_printf("OK!\n");

    neorv32_uart0_printf("PSRAM: Scrittura inizio a 0x%x ", current_ptr);

    while (bytes_to_read > 0) {
        WORD chunk = (bytes_to_read > sizeof(buffer)) ? (WORD)sizeof(buffer) : (WORD)bytes_to_read;
        
        res = pf_read(buffer, chunk, &br);
        if (res != FR_OK) {
            neorv32_uart0_printf("\nSD: Errore lettura a offset %d\n", total_written);
            break;
        }
        if (br == 0) break;

        for (UINT i = 0; i < br; i++) {
            // Scrittura volatile sulla PSRAM (Configurata a 210 gradi)
            *(volatile uint8_t *)(current_ptr + i) = buffer[i];
        }

        current_ptr += br;
        bytes_to_read -= (uint16_t)br;
        total_written += (uint32_t)br;
        
        // Feedback visivo ogni 32 byte
        neorv32_uart0_printf("."); 
    }

    neorv32_uart0_printf("\nPSRAM: Caricamento finito. Byte totali: %d\n", total_written);

    // --- DIAGNOSTICA INTEGRITA' ---
    neorv32_uart0_printf("PSRAM: Verifica primi 5 byte (Carattere 0):\n");
    for (uint32_t j = 0; j < 5; j++) {
        uint8_t val = *(volatile uint8_t *)(0x90000000 + j);
        neorv32_uart0_printf("[%d]: 0x%x ", j, (uint32_t)val);
    }
    neorv32_uart0_printf("\n");

    return res;
}

void vga_set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

/***************************************************************************************
** Function name:           setTextFont
** Description:             Set the font for the print stream
***************************************************************************************/
void vga_setTextFont(uint8_t f)
{
  textfont = (f > 0) ? f : 1; // Don't allow font 0
}

/***************************************************************************************
** Function name:           setTextSize
** Description:             Set the text size multiplier
***************************************************************************************/
void vga_setTextSize(uint8_t s)
{
  if (s>7) s = 7; // Limit the maximum size multiplier so byte variables can be used for rendering
  textsize = (s > 0) ? s : 1; // Don't allow font size 0
}

/***************************************************************************************
** Function name:           setTextColor
** Description:             Set the font foreground and background colour
***************************************************************************************/
void vga_setTextColor(uint16_t c, uint16_t b)
{
  textcolor   = c;
  textbgcolor = b;
}

/***************************************************************************************
** Function name:           vga_print
** Description:            Invia una stringa al display carattere per carattere
***************************************************************************************/
void vga_Print(const char *str) {
    while(*str) {
        // Chiama la funzione che gestisce il disegno e il cursore
        vga_write((uint8_t)*str++);
    }
}

size_t vga_write(uint8_t uniCode)
{
    // 1. Gestione rapida del Carriage Return
    if (uniCode == '\r') return 1;

    unsigned int width = 0;
    unsigned int height = 0;

    // --- GESTIONE FONT 1 (In PSRAM a 0x90000000) ---
    if (textfont == 1) 
    {
        // Larghezza fissa per il font 5x7 + 1 pixel di spazio
        width = 6; 
        height = 8;
    }
    else 
    {
        // Se il carattere è un controllo, usciamo
        if (uniCode < 32) return 0;

#ifdef LOAD_FONT2
        if (textfont == 2)
        {
            // Calcolo larghezza font 2 (proporzionale)
            width = widtbl_f16[uniCode - 32];
            height = (unsigned int)chr_hgt_f16;
            
            // Allineamento al byte per il rendering veloce
            width = (width + 6) / 8;
            width = width * 8; 
        }
    #ifdef LOAD_RLE
        else
    #endif
#endif

#ifdef LOAD_RLE
        {
            // Calcolo per font RLE
            width = pgm_read_byte(pgm_read_word(&(fontdata[textfont].widthtbl)) + uniCode - 32);
            height = pgm_read_byte(&fontdata[textfont].height);
        }
#endif
    }

    // Applichiamo la scalatura del testo
    height = height * textsize;

    // 2. Gestione New Line
    if (uniCode == '\n') {
        cursor_y += height;
        cursor_x = 0;
    }
    else
    {
        // 3. Gestione Text Wrap (640 pixel di risoluzione orizzontale)
        if (textwrap && (cursor_x + (width * textsize) >= 640))
        {
            cursor_y += height;
            cursor_x = 0;
        }

        // 4. DISEGNO FISICO E AGGIORNAMENTO CURSORE
        // La vga_drawChar ora gestisce sia la Flash che la PSRAM a 0x90000000
        cursor_x += vga_drawChar(uniCode, cursor_x, cursor_y, textfont);
    }

    return 1;
}

/***************************************************************************************
** Function name:           vga_drawCharGL
** Description:             Draw a single character in the Adafruit GLCD font (5x7)
***************************************************************************************/
void vga_drawCharGL(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size)
{
#ifdef LOAD_GLCD
    // 1. Clipping: Protezione per non uscire dai bordi della VGA
    if ((x >= 640) || (y >= 480) || ((x + 6 * size - 1) < 0) || ((y + 8 * size - 1) < 0)) return;

    bool fillbg = (bg != color);

    //uint32_t psram_font_ptr = 0x90000000 + (uint32_t)(c * 5);

    // 2. Ciclo principale sulle 6 colonne (5 font + 1 spazio)
    for (int8_t i = 0; i < 6; i++) {
        uint8_t line;
        if (i == 5) line = 0x0; // Spazio tra i caratteri
        else        line = font[(c * 5) + i];

        // 3. Ciclo sugli 8 bit della colonna (Altezza carattere)
        for (int8_t j = 0; j < 8; j++) {
            if (line & 0x1) {
                // PIXEL ATTIVO
                if (size == 1) {
                    vga_put_pixel(x + i, y + j, color);
                } else {
                    // Gestione Scalata (es. Size 2, 3...)
                    vga_fillRect(x + (i * size), y + (j * size), size, size, color);
                }
            } 
            else if (fillbg) {
                // SFONDO (Solo se bg != color, ovvero NO trasparenza)
                if (size == 1) {
                    vga_put_pixel(x + i, y + j, bg);
                } else {
                    vga_fillRect(x + (i * size), y + (j * size), size, size, bg);
                }
            }
            line >>= 1; // Scorri al bit successivo della colonna
        }
    }
#endif
}

int vga_drawChar(unsigned int uniCode, int x, int y, int font) {
    const unsigned char *ptr_font;
    int width = 0;
    int height = 0;
    unsigned int flash_address = 0; 
    int w_bytes;
    unsigned char line;
    int i, k, bit; // Variabili per i cicli for
    int current_y, current_x, base_x, pX;
    uint16_t color;

    // Gestione Font 1 (GLCD)
    if (font == 1) {
        #ifdef LOAD_GLCD
            vga_drawCharGL(x, y, uniCode, textcolor, textbgcolor, textsize);
            return 6 * textsize;
        #else
            return 0;
        #endif
    }

    if (uniCode < 32) return 0;
    uniCode -= 32;

        #ifdef LOAD_FONT2
        if (font == 2) {
            flash_address = (unsigned int)chrtbl_f16[uniCode];
            width = (int)widtbl_f16[uniCode];
            height = (int)chr_hgt_f16;
        }
        #endif
    

    w_bytes = (width + 7) / 8;
    line = 0;
    // Castiamo l'indirizzo (che sia Flash o PSRAM) al puntatore usato dai cicli
    ptr_font = (const unsigned char *)flash_address;

    // --- CASO 1: Font Scalato o Trasparente (Lento) ---
    if (textcolor == textbgcolor || textsize != 1) {
        for (i = 0; i < height; i++) {
            if (textcolor != textbgcolor) {
                vga_fillRect(x, y + (i * textsize), width * textsize, textsize, textbgcolor);
            }

            for (k = 0; k < w_bytes; k++) {
                // Lettura da PSRAM o Flash (il controller gestisce il mapping a 0x90000000)
                line = ptr_font[w_bytes * i + k];
                
                if (line) {
                    pX = x + (k * 8 * textsize);
                    for (bit = 0; bit < 8; bit++) {
                        if (line & (0x80 >> bit)) {
                            if (textsize == 1) {
                                vga_put_pixel(pX + bit, y + i, textcolor);
                            } else {
                                //vga_fillRect(pX + bit * textsize, y + i * textsize, textsize, textsize, textcolor);
                            }
                        }
                    }
                }
            }
        }
    } 
    // --- CASO 2: Font Standard (Veloce) ---
    else {
        for (i = 0; i < height; i++) {
            current_y = y + i;
            
            while(VGA_IS_BUSY);
            VGA_REG_Y = current_y;

            for (k = 0; k < w_bytes; k++) {
                // Lettura da PSRAM o Flash
                line = ptr_font[w_bytes * i + k];
                base_x = x + (k * 8);

                for (bit = 0; bit < 8; bit++) {
                    if ((k * 8) + bit >= width) break;

                    current_x = base_x + bit;
                    color = (line & (0x80 >> bit)) ? textcolor : textbgcolor;

                    while(VGA_IS_BUSY);
                    VGA_REG_X = current_x;
                    VGA_REG_COLOR = color;
                    VGA_STROBE = 1;
                }
            }
        }
    }

    return width * textsize;
}

/********************************************************** */
// Variabili di stato globali
int hardware_offset = 0;
int terminal_row = 0;
int terminal_col = 0; // Nuova variabile per tracciare la colonna
int font_h = 16;
int font_w = 8;       // Larghezza standard font
int max_rows;

void vga_terminal_init(int h) {
    font_h = h;
    max_rows = 480 / font_h;
    terminal_row = 0;
    terminal_col = 0;
    hardware_offset = 0;
    VGA_REG_SCROLL = 0;
    vga_fillRect(0, 0, 640, 480, 0x0000);
}

void vga_terminal_putc(char c) {
    // 1. Gestione del ritorno a capo (INVIO o fine riga)
    if (c == '\r' || c == '\n' || terminal_col >= (640 / font_w)) {
        terminal_col = 0;
        terminal_row++;

        // 2. Controllo Scroll Hardware
        if (terminal_row >= max_rows) {
            hardware_offset = (hardware_offset + font_h) % 480;
            VGA_REG_SCROLL = hardware_offset;

            // Pulizia della nuova riga rullata dal basso
            int y_to_clear = (hardware_offset + ((max_rows - 1) * font_h)) % 480;
            vga_fillRect(0, y_to_clear, 640, font_h, 0x0000);
            
            terminal_row = max_rows - 1;
        }
        
        // Se era solo un carattere di fine riga automatica, usciamo qui
        if (c == '\r' || c == '\n') return;
    }

    // 3. Stampa del carattere singolo
    if (c >= 32 && c <= 126) { // Solo caratteri stampabili
        char str[2] = {c, '\0'};
        int x_phys = terminal_col * font_w;
        int y_phys = (hardware_offset + (terminal_row * font_h)) % 480;

        vga_set_cursor(x_phys, y_phys);
        vga_Print(str);
        
        terminal_col++;
    }
}

/**
 * Stampa una stringa sul terminale VGA carattere per carattere.
 * Gestisce automaticamente lo scroll e il wrapping.
 */
void vga_terminal_print(const char* s) {
    while (*s) {
        vga_terminal_putc(*s++);
    }
}

/**
 * Stampa una stringa e aggiunge un ritorno a capo.
 */
void vga_terminal_println(const char* s) {
    vga_terminal_print(s);
    vga_terminal_putc('\n');
}


