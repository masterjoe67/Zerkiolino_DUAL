#include "doomgeneric.h"
#include "neorv32.h"
#include "doomgeneric_neorv32.h"
#include "../petit_fatfs/pff.h"
#include "../vga/vga.h"

// Dove carichiamo il WAD nella SDRAM 1
#define WAD_SDRAM_ADDR 0x91000000 



void I_GetEvent(void);

void DG_Init() {
    // 1. Inizializza le tue periferiche (se non già fatto nel bootloader)
    // neorv32_uart0_printf("DG_Init: Caricamento WAD...\n");

    // 2. Caricamento integrale del WAD in SDRAM 1
    UINT br;
    FRESULT res = pf_open("DOOM1.WAD");
    if (res == FR_OK) {
        // Carichiamo circa 4MB (dimensione tipica dello shareware)
        pf_read((void*)WAD_SDRAM_ADDR, 4*1024*1024, &br);
    } else {
        // Errore critico: se non c'è il WAD, Doom non parte
        while(1); 
    }
}



void DG_SleepMs(uint32_t ms) {
    neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), ms);
}

uint32_t DG_GetTicksMs() {
    // Calcoliamo i millisecondi dai cicli di clock a 100MHz
    uint64_t mtime = neorv32_clint_time_get();
    return (uint32_t)(mtime / 100000);
}

int DG_GetKey(int* outKey, unsigned char* outIsDown) {
    
    // Esempio di logica se leggi dalla UART o dai tasti della DE0-Nano
    // Se c'è un tasto:
    // *outKey = ...;      // Il codice del tasto (es. 'w', 'a', 's', 'd' o costanti Doom)
    // *outIsDown = 1;     // 1 per premuto, 0 per rilasciato
    // return 1;           // Ritorna 1 se hai trovato un evento tasto
    
    return 0; // Ritorna 0 se non succede nulla
}

void vga_blit_to_subsystem(uint8_t* source, uint32_t size) {
    // 1. Seleziona la pagina video di destinazione sul Nodo 2
    // vga_select_subsystem_page(current_back_buffer);

    // 2. Trasferimento dati
    // Qui devi usare il ciclo che usi per le foto. 
    // Esempio se scrivi su un bus parallelo mappato in IO:
    for (uint32_t i = 0; i < size; i++) {
        // Scrivi il byte i-esimo sul bus verso la seconda Nano
        // BUS_DATA_REG = source[i]; 
        // STROBE_PULSE();
    }
}

static uint8_t back_buffer_page = 1; // Pagina su cui DOOM disegna
static uint8_t front_buffer_page = 0; // Pagina visualizzata sul monitor

void DG_DrawFrame2() {
    I_GetEvent();
    // 1. Puntiamo alla pagina di scrittura (Back Buffer)
    vga_set_write_page(back_buffer_page);

    // 2. Il buffer è già RGB565 e già scalato a 640x400 da I_FinishUpdate
    uint16_t *pixel_buffer = (uint16_t *)DG_ScreenBuffer; 

    uint16_t *pixel_ptr = (uint16_t *)pixel_buffer;

    // Ciclo sulle dimensioni REALI del buffer di uscita
    VGA_REG_MODE = 0x0001; // Modalità di scrittura rapida (es. auto-incremento)
    for (uint16_t y = 0; y < 400; y++) {
        VGA_REG_Y = y;
        VGA_REG_X = 0; // Iniziamo da x=0 per ogni riga
        for (uint16_t x = 0; x < 640; x++) {
            // Leggiamo il pixel (x, y) dal buffer scalato 640x400
            // L'indice deve riflettere la larghezza reale (640)
            //uint16_t color = pixel_buffer[y * 640 + x];
                uint16_t color = *pixel_ptr++;
                //while (VGA_IS_BUSY);
                //VGA_REG_X = x;
                VGA_REG_COLOR = color; // Invia i 16 bit colore in un colpo solo
    
                VGA_STROBE = 1; // Lo strobe fa scattare il mapping Bank0/Bank1 nel VHDL

            // Inviamo il pixel alla VGA nelle coordinate reali
            //vga_put_pixel(x, y, color);
        }
    }
    VGA_REG_MODE = 0x0000; // Torniamo alla modalità normale
    // 3. Flip: visualizziamo la pagina appena riempita
    vga_set_read_page(back_buffer_page);

    // 4. Swap: scambiamo le pagine per il prossimo frame
    uint8_t temp = front_buffer_page;
    front_buffer_page = back_buffer_page;
    back_buffer_page = temp;
}

void DG_DrawFrame() {
    I_GetEvent();

    // 1. Puntiamo alla pagina di scrittura (Back Buffer)
    vga_set_write_page(back_buffer_page);

    // Il buffer ora è 320x200 (grazie alla modifica in I_FinishUpdate)
    uint16_t *pixel_ptr = (uint16_t *)DG_ScreenBuffer; 

    // --- CONFIGURAZIONE HARDWARE ---
    // Bit 0: Auto-incremento X attivo
    // Bit 1: Scaling Orizzontale 2x attivo
    // Bit 2: Scaling Verticale 2x attivo
    // Totale: 0x0007 (o 0x0001 se vuoi solo auto-inc, ma noi vogliamo lo scaling!)
    VGA_REG_MODE = 0x0007; 

    // Ciclo sulle dimensioni ORIGINALI di Doom (320x200)
    for (uint16_t y = 0; y < 200; y++) {
        
        while (VGA_IS_BUSY); // Aspetta che l'hardware sia pronto per la riga
        
        VGA_REG_Y = y;
        VGA_REG_X = 0; 

        for (uint16_t x = 0; x < 320; x++) {
            // Inviamo il pixel. L'auto-incremento sposterà X da solo.
            VGA_REG_COLOR = *pixel_ptr++; 
            VGA_STROBE = 1; 
        }
    }

    // Torniamo alla modalità standard (opzionale, dipende se vuoi lo scaling sempre attivo)
    // Se lo lasci attivo (0x0006), anche il resto dell'interfaccia sarà scalato correttamente.
    // VGA_REG_MODE = 0x0000; 

    // 3. Flip e 4. Swap
    vga_set_read_page(back_buffer_page);

    uint8_t temp = front_buffer_page;
    front_buffer_page = back_buffer_page;
    back_buffer_page = temp;
}



void DG_SetWindowTitle(const char * title) {
    // Sul NEORV32 non abbiamo finestre, quindi usiamo la UART
    // per monitorare lo stato del gioco (es. "DOOM - Level 1: Hangar")
    if (title != NULL) {
        neorv32_uart0_printf("\n[GAME STATE]: %s\n", title);
    }
}

// Stub per mkdir: PetitFS non supporta la creazione di directory.
// Restituiamo 0 (successo finto) per far andare avanti il gioco.
int mkdir(const char *pathname, int mode) {
    return 0; 
}



