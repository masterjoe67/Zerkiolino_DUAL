#define __MAIN_C__

/*=======================================================================*/
/* Includes                                                              */
/*=======================================================================*/
#include "neorv32.h"
#include "terminal.h"
#include "neorv32_spi.h"
#include "petit_fatfs/pff.h"
#include "vga/vga.h"
#include "PS2_Keyboard/ps2_kbd.h"
#include <math.h>

/*=======================================================================*/
/* Configuration & Constants                                             */
/*=======================================================================*/
#define term_printf neorv32_uart0_printf

// Indirizzo base della SDRAM (dal tuo setup: 0x90000000)
#define PSRAM_BASE    0x90000000
#define PSRAM_PTR     ((volatile uint32_t*) PSRAM_BASE)

/*=======================================================================*/
/* Global Variables                                                      */
/*=======================================================================*/
FATFS fs;          // Buffer file system
WORD br;           // Byte letti effettivamente

/*=======================================================================*/
/* Helper Procedures                                                     */
/*=======================================================================*/

// Stampa un byte in formato esadecimale sulla UART
void uart_put_hex(uint8_t data) {
    const char hex_chars[] = "0123456789ABCDEF";
    neorv32_uart0_putc('0');
    neorv32_uart0_putc('x');
    neorv32_uart0_putc(hex_chars[(data >> 4) & 0x0F]);
    neorv32_uart0_putc(hex_chars[data & 0x0F]);
}

static void OutputBootMessage (void) {
    const char ResetScreen[] = { 0x1B, 'c', 0 };
    term_printf("%s", ResetScreen);
    neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 200);

    term_printf("\r\n*********************************");
    term_printf("\r\n* Zerkiolino RISC-V BOOT     *");
    term_printf("\r\n*********************************\r\n");
}

static int neorv32_Init (void) {
    term_Start();
    if (0 == neorv32_gpio_available()) {
        term_printf("\r\nError! No GPIO unit synthesized!\r\n");
        return(-1);
    }
    neorv32_rte_setup();
    return(0);   
}

void setup_sd(void) {
    FRESULT res;
    neorv32_uart0_puts("SD Init... ");
    
    // Inizializza il file system (chiama internamente disk_initialize)
    res = pf_mount(&fs);

    if (res == FR_OK) {
        neorv32_uart0_puts("OK!\n");
    } else {
        neorv32_uart0_puts("FAIL! Code: ");
        uart_put_hex(res);
        neorv32_uart0_puts("\n");
    }
}







void keyboard_demo() {
ps2_event_t ev;
    neorv32_uart0_printf("Sistema Pronto. Joe, premi i tasti!\n");

    while(1) {
        if (ps2_get_event(&ev)) {
            // Usiamo stringhe semplici per evitare errori di formattazione %x
            neorv32_uart0_puts("Tasto: ");
            uart_put_hex(ev.scancode);
            
            if (ev.state == PS2_KEY_PRESSED) {
                neorv32_uart0_puts(" | STATO: PREMUTO ");
            } else {
                neorv32_uart0_puts(" | STATO: RILASCIATO");
            }

            if (ev.extended) {
                neorv32_uart0_puts(" | EXT: SI\n");
            } else {
                neorv32_uart0_puts(" | EXT: NO\n");
            }
        }
    }
}
/******************************************************************************
 * Test nuove routine VGA
 * 
 **************************************************************************/


void test_autoincrement() {
    uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFE0, 0xF81F}; // R, G, B, Y, M
    
    // 1. Disabilita autoincremento (per sicurezza iniziale)
    VGA_REG_MODE = 0x0000; // Cmd 6 con dato 0 disabilita autoincremento

    // 2. Imposta coordinate di partenza (X=0, Y=100)

    VGA_REG_X = 0;
    VGA_REG_Y = 100;
    // vga_send_cmd(1, 0);   // Cmd 001: Set X
    // vga_send_cmd(2, 100); // Cmd 010: Set Y

    // 3. Abilita Autoincremento (REG_MODE bit 0 = 1)
    VGA_REG_MODE = 0x0001; 

    // 4. Spara 500 pixel di fila
    // Se l'hardware funziona, vedrai una linea multicolore lunga 500px
    for(int i = 0; i < 500; i++) {
        // Cambia colore ogni 100 pixel per vedere il gradiente
        uint16_t current_color = colors[(i/100) % 5];
        
        // Aspetta che la FIFO non sia piena (se hai il segnale busy mappato)
        // while(VGA_IS_BUSY); 
        VGA_REG_COLOR = current_color;
        VGA_STROBE = 1;
    }
    
    // 5. Disabilita autoincremento a fine test
    VGA_REG_MODE = 0x0000;
}

void test_vga_scaling() {
    // 1. Reset e configurazione: Auto-increment (bit 0) + Scaling H (bit 1) + Scaling V (bit 2)
    // 0000 0111 binario = 0x07
    while (VGA_IS_BUSY);
    VGA_REG_MODE = 0x07; 

    // 2. Pulizia schermo veloce (320x200 ora riempie tutto il 640x400 grazie allo scaling)
    for (int y = 0; y < 200; y++) {
        while (VGA_IS_BUSY);
        VGA_REG_X = 0;
        VGA_REG_Y = y;
        for (int x = 0; x < 320; x++) {
            while (VGA_IS_BUSY);
            VGA_REG_COLOR = 0x0000; // Nero
            VGA_STROBE = 1;
        }
    }

    // 3. Disegno pattern di test (punti alternati)
    // Se lo scaling funziona, vedrai dei blocchi 2x2 pixel sul monitor
    for (int y = 50; y < 150; y++) {
        while (VGA_IS_BUSY);
        VGA_REG_X = 110;
        VGA_REG_Y = y;
        for (int x = 0; x < 100; x++) {
            while (VGA_IS_BUSY);
            
            // Colore alternato per creare una scacchiera
            if ((x + y) % 2 == 0) {
                VGA_REG_COLOR = 0xFFFF; // Bianco
            } else {
                VGA_REG_COLOR = 0xF800; // Rosso
            }
            
            VGA_STROBE = 1; // Grazie all'auto-incremento X avanza da solo
        }
    }
}

// Parametri della funzione
#define SCALE 20.0f
#define SPEED 0.1f

void draw_3d_function(float time) {
    int last_sx = -1, last_sy = -1;

    // Cicliamo su una griglia X, Y "mondo"
    for (float x = -10.0f; x < 10.0f; x += 0.4f) {
        for (float y = -10.0f; y < 10.0f; y += 0.4f) {
            
            // 1. Calcolo della funzione (Sinc circolare)
            float d = sqrtf(x*x + y*y);
            float z = 5.0f * sinf(d - time) / (d + 0.5f); // +0.5 per evitare div by zero

            // 2. Proiezione Isometrica Semplice (3D -> 2D)
            // Trasformiamo coordinate (x, y, z) in (screen_x, screen_y)
            int sx = (int)((x - y) * cosf(0.523f) * SCALE) + (SCREEN_WIDTH / 2);
            int sy = (int)((x + y) * sinf(0.523f) * SCALE - (z * SCALE)) + (SCREEN_HEIGHT / 2);

            // 3. Shading base (opzionale)
            // Più è alto Z, più il colore è tendente al rosso/giallo
            uint16_t color = (uint16_t)(z * 1000) + 0x07E0; // Esempio colore dinamico

            if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT) {
                vga_put_pixel(sx, sy, color);
            }
        }
    }
}

static uint8_t back_buffer_page = 1; // Pagina su cui DOOM disegna
static uint8_t front_buffer_page = 0; // Pagina visualizzata sul monitor

/*=======================================================================*/
/* Main Loop                                                             */
/*=======================================================================*/
int main (void) {
    // 1. Hardware Init
    if (neorv32_Init() != 0) return(1);
    setup_sd();
    // 2. Welcome Message
    OutputBootMessage();
    neorv32_gpio_port_set(0);
    vga_set_write_page(0);
    vga_set_read_page(0);
    //load_font_to_psram();
    vga_setTextFont(1);
    vga_setTextSize(2);
    vga_setTextColor(RED, 0x0000);
    vga_set_cursor(0, 0);
    vga_clear(BLACK);

    vga_Print("Zerkiolino RISC-V VGA Test ");


    float t = 0;
    while(1) {
        vga_set_write_page(back_buffer_page);
        vga_clear(BLACK); // Pulisce il buffer SDRAM
        draw_3d_function(t);
        t += SPEED;
        vga_set_read_page(back_buffer_page);
        uint8_t temp = front_buffer_page;
        front_buffer_page = back_buffer_page;
        back_buffer_page = temp;
    }


    test_vga_scaling();
    
    test_autoincrement();
    ps2_event_t ev;
    while(1) {
        if (ps2_get_event(&ev)) {
            if (ev.state == PS2_KEY_PRESSED) {
                vga_clear(BLACK);
                test_vga_scaling();

            }
        }
    }


    while (1) {

        keyboard_demo();
    }


//while (1);
  //vga_load_image_pfs("ZERK.BIN"); // Carica un'immagine da SD (assicurati che sia in RGB565 e 640x480)
    //neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 2000);
   //vga_load_image_pfs("ZERK.BIN"); // Carica un'immagine da SD (assicurati che sia in RGB565 e 640x480)


   neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 1000);




    int i = 0;
    while (1) {
        neorv32_gpio_port_set(1 << i);
        i = (i + 1) % 8;
        neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 200);
    }   
    
    return(0);
}