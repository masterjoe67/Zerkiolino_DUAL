#define __MAIN_C__

/*=======================================================================*/
/* Includes                                                              */
/*=======================================================================*/
#include "neorv32.h"
#include "terminal.h"
#include "neorv32_spi.h"
#include "petit_fatfs/pff.h"
#include "vga/vga.h"

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

int test_psram_integrity_old() {
    uint32_t test_pattern = 0xABCDEF12;
    uint32_t read_back;

    term_printf("\n--- PSRAM Integrity Test ---\n");

    // Scrittura
    term_printf("Scrittura in corso a 0x%x: 0x%x\n", (uint32_t)PSRAM_PTR, test_pattern);
    *PSRAM_PTR = test_pattern;

    // Piccola attesa opzionale per stabilità
    neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 10);

    // Lettura
    read_back = *PSRAM_PTR;
    term_printf("Rilettura effettuata: 0x%x\n", read_back);

    // Analisi dei singoli byte (utile per capire quale ciclo QPI fallisce)
    if (read_back == test_pattern) {
        term_printf("RISULTATO: [OK] - Sincronizzazione 4+4 QPI perfetta!\n");
        return 1;
    } else {
        term_printf("RISULTATO: [FAIL] - Errore di integrità!\n");
        
        // Debug bitwise: aiuta a capire se perdi il nibble alto o basso
        term_printf("Differenza: 0x%x\n", test_pattern ^ read_back);
        
        if ((read_back & 0xFFFF) != (test_pattern & 0xFFFF)) {
            term_printf("Nota: L'errore sembra nei primi 2 byte (cicli 0-1).\n");
        }
        
        return 0;
    }
}

int test_psram_integrity() {
    // La variabile static mantiene il valore tra una chiamata e l'altra della funzione
    static uint32_t incremental_pattern = 0x0f110f11;
    uint32_t read_back;
    int errors = 0;
    int num_tests = 10; // Quante locazioni testare ad ogni chiamata

    term_printf("\n--- PSRAM Incremental Test  ---\n", 60);

    for (int i = 0; i < num_tests; i++) {
        uint32_t current_pattern = incremental_pattern;
        // Testiamo indirizzi consecutivi a partire dalla base della PSRAM
        volatile uint32_t *target_addr = PSRAM_PTR + i;

        // 1. Scrittura
        *target_addr = current_pattern;

        // Piccola attesa per stabilità (come nel tuo codice originale)
        neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 1);

        // 2. Lettura
        read_back = *target_addr;

        if (read_back == current_pattern) {
            term_printf("[%d] Addr 0x%x: OK (0x%x)\n", i, (uint32_t)target_addr, read_back);
        } else {
            term_printf("[%d] Addr 0x%x: FAIL! Scritto: 0x%x, Letto: 0x%x\n", 
                        i, (uint32_t)target_addr, current_pattern, read_back);
            term_printf("    XOR Diff: 0x%x\n", current_pattern ^ read_back);
            errors++;
        }

        // Incrementiamo il pattern per il prossimo indirizzo/ciclo
        incremental_pattern++;
    }

    if (errors == 0) {
        term_printf("RISULTATO FINALE: [OK] - Tutti i pattern incrementali corretti.\n");
        return 1;
    } else {
        term_printf("RISULTATO FINALE: [FAIL] - %d errori rilevati.\n", errors);
        return 0;
    }
}

FRESULT load_image_to_psram(const char* filename, uint32_t destination_addr) {
    FRESULT res;
    UINT br;
    // Buffer a 32 bit per la massima velocità sul bus Wishbone
    uint32_t buffer[128]; // 512 byte (un settore SD)
    uint32_t *psram_ptr = (uint32_t*)destination_addr;

    neorv32_uart0_printf("SD: Apertura con PetitFS: %s\n", filename);

    // PetitFS non usa l'oggetto FIL, apre il file globalmente
    res = pf_open(filename);
    if (res != FR_OK) {
        neorv32_uart0_printf("Errore apertura: %d\n", res);
        return res;
    }

    neorv32_uart0_printf("PSRAM: Scrittura ");

    while (1) {
        // Lettura settore da PetitFS
        res = pf_read(buffer, 512, &br);
        
        if (res != FR_OK || br == 0) break;

        // Calcoliamo quante word da 32 bit scrivere
        // Usiamo (br+3)/4 per gestire eventuali blocchi finali non multipli di 4
        uint32_t words_to_write = (br + 3) / 4;

        for (uint32_t i = 0; i < words_to_write; i++) {
            // Il NEORV32 spara la word al tuo controller VHDL
            *psram_ptr++ = buffer[i];
        }

     
        if (br < 512) break; // Fine del file
    }

    neorv32_uart0_printf("\nCompletato.\n");
    return res;
}

void vga_display_from_psram(uint32_t psram_src_addr) {
    uint16_t x = 0, y = 0;
    
    // Puntatore alla sorgente in PSRAM (es. 0x90000000)
    // Usiamo uint16_t perché l'immagine è a 16 bit per pixel (2 byte)
    uint16_t *psram_ptr = (uint16_t *)psram_src_addr;

    while (y < 480) {
        // Leggiamo il pixel direttamente dalla PSRAM
        uint16_t color = *psram_ptr++;

        // Lo spariamo nel VGA (che immagino scriva nella BRAM interna o in un altro buffer)
        vga_put_pixel(x, y, color);

        x++;
        if (x >= 640) {
            x = 0;
            y++;
            if (y >= 480) break;
        }
    }
}






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
    load_font_to_psram();
    vga_setTextFont(1);
    vga_setTextSize(2);
    vga_setTextColor(RED, 0x0000);
    vga_set_cursor(0, 0);
vga_clear(BLACK);
    vga_Print("Zerkiolino RISC-V VGA Test ");

    while (1) {
        test_psram_integrity();
        neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 500);
    }


//while (1);
  //vga_load_image_pfs("ZERK.BIN"); // Carica un'immagine da SD (assicurati che sia in RGB565 e 640x480)
    //neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 2000);
   //vga_load_image_pfs("ZERK.BIN"); // Carica un'immagine da SD (assicurati che sia in RGB565 e 640x480)
load_image_to_psram("ZERK.BIN", PSRAM_BASE); // Carica l'immagine direttamente in PSRAM
vga_display_from_psram(PSRAM_BASE); // Visualizza l'immagine caricata

   neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 1000);




    int i = 0;
    while (1) {
        neorv32_gpio_port_set(1 << i);
        i = (i + 1) % 8;
        neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 200);
    }   
    
    return(0);
}