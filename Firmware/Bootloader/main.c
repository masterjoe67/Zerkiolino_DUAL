#define __MAIN_C__

/*=======================================================================*/
/* Includes                                                              */
/*=======================================================================*/
#include "neorv32.h"
#include "terminal.h"
#include "neorv32_spi.h"
#include "petit_fatfs/pff.h"

/*=======================================================================*/
/* Configuration & Constants                                             */
/*=======================================================================*/
#define term_printf neorv32_uart0_printf

// Indirizzo base della SDRAM (dal tuo setup: 0x90000000)
#define SDRAM_BASE_ADDR 0x90000000
#define MAX_LOAD_SIZE   (1024 * 1024) // Limite caricamento 1MB per sicurezza
#define EXE_HEADER_SIZE 12            // 3 parole da 4 byte (Signature, Size, Checksum)

/*=======================================================================*/
/* Global Variables                                                      */
/*=======================================================================*/
FATFS fs;          

/*=======================================================================*/
/* Helper Procedures                                                     */
/*=======================================================================*/

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
    term_printf("\r\n* Zerkiolino SD-SDRAM BOOT      *");
    term_printf("\r\n*********************************\r\n");
}

static int neorv32_Init (void) {
    term_Start();
    if (0 == neorv32_gpio_available()) return(-1);
    neorv32_rte_setup();
    return(0);   
}

/*=======================================================================*/
/* SD Card & Boot Logic                                                  */
/*=======================================================================*/

void setup_sd(void) {
    FRESULT res;
    neorv32_uart0_puts("SD Init... ");
    res = pf_mount(&fs);
    if (res == FR_OK) {
        neorv32_uart0_puts("OK!\n");
    } else {
        neorv32_uart0_puts("FAIL! Code: ");
        uart_put_hex(res);
        neorv32_uart0_puts("\n");
    }
}

void boot_from_sd(void) {
    FRESULT res;
    UINT br; 
    uint32_t header_buffer[3]; // Buffer per scartare i 12 byte di metadati
    void (*user_app)(void) = (void*)SDRAM_BASE_ADDR;

    neorv32_uart0_puts("Opening BOOT.BIN... ");
    res = pf_open("BOOT.BIN");
    if (res != FR_OK) {
        neorv32_uart0_puts("NOT FOUND!\n");
        return;
    }

    // 1. ELIMINAZIONE METADATI
    // Leggiamo i primi 12 byte (Signature, Size, Checksum) e li ignoriamo
    res = pf_read(header_buffer, EXE_HEADER_SIZE, &br);
    if (res != FR_OK || br != EXE_HEADER_SIZE) {
        neorv32_uart0_puts("ERROR: Header read failed!\n");
        return;
    }
    
    // Piccolo debug: stampa la signature trovata (dovrebbe essere 0xB007C0DE)
    neorv32_uart0_puts("Signature: ");
    neorv32_uart0_printf("%x", header_buffer[0]); 
    neorv32_uart0_puts("\n");

    // 2. CARICAMENTO CODICE REALE
    // La testina del file è ora al byte 12. Carichiamo il resto in SDRAM.
    neorv32_uart0_puts("Loading Code to SDRAM... ");
    
    // Aumentiamo velocità SPI prima del carico pesante
    neorv32_spi_setup(CLK_PRSC_4, 0, 0, 0);

    res = pf_read((void*)SDRAM_BASE_ADDR, MAX_LOAD_SIZE, &br);

    if (res == FR_OK && br > 0) {
        neorv32_uart0_puts("Done.\nLaunching Firmware...\n");
        neorv32_uart0_puts("---------------------------------\n");

        while (neorv32_uart0_tx_busy());
        
        // Sincronizzazione fondamentale per svuotare le cache istruzioni
        asm volatile ("fence.i");

        // JUMP alla SDRAM dove ora c'è il codice pulito!
        user_app();
        
    } else {
        neorv32_uart0_puts("ERROR: Read failed!\n");
    }
}

/*=======================================================================*/
/* Main Loop                                                             */
/*=======================================================================*/
int main (void) {
    if (neorv32_Init() != 0) return(1);

    OutputBootMessage();
    neorv32_gpio_port_set(0);
    
    setup_sd();
    boot_from_sd();
    
    // Se arriviamo qui, il boot è fallito
    term_printf("\r\nSystem halted. Check SD card.\r\n");

    int i = 0;
    while (1) {
        neorv32_gpio_port_set(1 << i);
        i = (i + 1) % 8;
        neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 100);
    }   
    return(0);
}