#define __MAIN_C__

/*=======================================================================*/
/* Includes                                                              */
/*=======================================================================*/
#include "neorv32.h"
#include "terminal.h"
#include "neorv32_spi.h"
#include "neorv32_rte.h"
#include "neorv32_gptmr.h"
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


#define REG_AUDIO_ADDR (*(volatile uint32_t *)(VGA_BASE + 0x20))

void SB_SendWaveform(const uint8_t* buffer, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        // Se la FIFO è piena, l'hardware alza il bit busy (bit 0 di 0x80000000)
        // Il processore aspetta qui, sincronizzandosi automaticamente
        while (VGA_IS_BUSY);

        // Invio del campione (8 bit di Doom -> 12 bit DAC)
        uint16_t sample12 = (uint16_t)buffer[i] << 4;
        REG_AUDIO_ADDR = 0x3000 | (sample12 & 0x0FFF);
    }
}

// Impostazioni della nostra SoundBlaster
#define SAMPLE_RATE 11025
#define BUFFER_SIZE SAMPLE_RATE // Un buffer da esattamente 1 secondo (circa 11 KB)

// Il buffer in memoria RAM
uint8_t sound_buffer[BUFFER_SIZE];





void test_audio_real() {
    // Generiamo un suono di 1 secondo (11025 campioni)
    // Invece di caricarlo, lo calcoliamo matematicamente per semplicità di test
    // ma lo trattiamo come se fosse un campione PCM 8-bit.
    
    while(1) {
        for (int i = 0; i < 11025; i++) {
            // Se la FIFO è piena, aspetta (Polling)
            while (*(volatile uint32_t*)0x40000000 & 0x1); 

            // Crea un effetto "Laser" che scende di tono
            // (Simula un vero campione dinamico)
            uint8_t sample = (uint8_t)(128 + 127 * (i % (100 + i/100) > (50 + i/200) ? 1 : -1));
            
            // Invia al DAC
            REG_AUDIO_ADDR = 0x3000 | ((uint16_t)sample << 4);
        }
        
        // Pausa di 2 secondi tra un "bang" e l'altro
        neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 2000);
    }
}

/**
 * Pulisce una riga orizzontale di pixel (640 pixel)
 * @param y_phys: la coordinata Y fisica in memoria (0-479)
 */
void vga_clear_line(int y_phys) {
    // 1. Imposta la coordinata Y
    VGA_REG_Y = y_phys;
    
    // 2. Imposta X all'inizio
    VGA_REG_X = 0;
    
    // 3. Imposta il colore (Nero = 0x0000)
    VGA_REG_COLOR = 0x0000;

    // 4. Scrivi 640 pixel. 
    // Grazie all'auto-incremento X nel VHDL (reg_mode bit 0),
    // basta scrivere nel registro PIXEL ripetutamente.
    for (int x = 0; x < 640; x++) {
        VGA_REG_COLOR = 0x0000; 
    }
}

// Variabili globali per lo stato del terminale
/*int terminal_row = 0;      // Riga corrente (0-29)
int hardware_offset = 0;   // Offset di scroll attuale (0-479)
const int FONT_H = 16;     // Altezza del font*/

/*void terminal_println(char* text) {
    // 1. Calcoliamo dove scrivere fisicamente in memoria
    // La riga attuale visibile si trova all'offset hardware + (riga logica * altezza font)
    int y_phys = (hardware_offset + (terminal_row * FONT_H)) % 480;

    // 2. Posizioniamo il cursore e stampiamo
    vga_set_cursor(0, y_phys);
    vga_Print(text);

    // 3. Prepariamo la riga successiva
    if (terminal_row < 29) {
        // C'è ancora spazio sullo schermo, aumentiamo solo la riga logica
        terminal_row++;
    } else {
        // Siamo all'ultima riga: dobbiamo scrollare l'hardware
        // L'offset si sposta in avanti di una riga di font
        hardware_offset = (hardware_offset + FONT_H) % 480;
        VGA_REG_SCROLL = hardware_offset;

        // Ora dobbiamo pulire la riga che è appena "apparsa" in fondo al monitor.
        // Fisicamente, questa riga si trova esattamente dove finisce la visualizzazione.
        int y_to_clear = (hardware_offset + (29 * FONT_H)) % 480;
        
        // Usiamo la tua vga_fillRect ottimizzata
        vga_fillRect(0, y_to_clear, 640, FONT_H, 0x0000);
        
        // Restiamo sulla riga 29 (l'ultima)
        terminal_row = 29;
    }
}*/

/*void terminal_run2() {
    vga_Print("Zerkiolino Terminal Ready...\n");
    
    // Posizioniamo il cursore sulla riga successiva a quella del benvenuto
    terminal_row = 1; 

    while (1) {
        if (neorv32_uart0_available()) {
            char c = neorv32_uart0_getc();

            // 1. Echo sulla UART (per vedere sul PC cosa scrivi)
            neorv32_uart0_putc(c);

            // 2. Gestione INVIO
            if (c == '\r' || c == '\n') {
                terminal_row++;
                
                // Se superiamo il fondo dello schermo, scrolliamo
                if (terminal_row >= 30) {
                    hardware_offset = (hardware_offset + 16) % 480;
                    VGA_REG_SCROLL = hardware_offset;
                    
                    int y_to_clear = (hardware_offset + (29 * 16)) % 480;
                    vga_fillRect(0, y_to_clear, 640, 16, 0x0000);
                    
                    terminal_row = 29;
                }
                
                // Reset posizione X per la nuova riga
                vga_set_cursor(0, (hardware_offset + (terminal_row * 16)) % 480);
            } 
            // 3. Caratteri stampabili
            else if (c >= 32 && c <= 126) {
                char str[2] = {c, '\0'};
                
                // Calcoliamo la posizione Y attuale e stampiamo il singolo carattere
                int y_phys = (hardware_offset + (terminal_row * 16)) % 480;
                
                // Nota: vga_Print deve mantenere internamente la posizione X 
                // o devi aggiornare il cursore manualmente qui
                vga_Print(str); 
            }
        }
    }
}*/

void terminal_run() {
    vga_terminal_init(16); // Font 16px
    vga_terminal_println("Zerkiolino Shell Ready\n");

    while (1) {
        if (neorv32_uart0_available()) {
            char c = neorv32_uart0_getc();
            
            // Echo sulla UART per il PC
            neorv32_uart0_putc(c); 
            
            // Visualizzazione immediata su VGA
            vga_terminal_putc(c);
        }
    }
}

/*=======================================================================*/
/* Main Loop                                                             */
/*=======================================================================*/
int main (void) {
    neorv32_rte_setup();
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

    //vga_Print("Zerkiolino RISC-V VGA Test \n");
    vga_terminal_init(12);
    terminal_run();

    // 2. Loop di riproduzione infinito
    while(1) {
        // Passa l'intero array alla "scheda audio"
        SB_SendWaveform(sound_buffer, BUFFER_SIZE);
        
        // Nota magica: non serve nessun delay software qui!
        // Il processore girerà a tutta velocità, ma la riga 'while(busy)' 
        // dentro SB_SendWaveform farà da freno automatico, sincronizzando
        // l'esecuzione in C con il timer a 11kHz dell'FPGA.
    }
  
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





   neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 1000);




    int i = 0;
    while (1) {
        neorv32_gpio_port_set(1 << i);
        i = (i + 1) % 8;
        neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 200);
    }   
    
    return(0);
}
