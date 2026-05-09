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

/*=======================================================================*/
/* SD Card & Boot Logic                                                  */
/*=======================================================================*/

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

void boot_from_sd(void) {
    FRESULT res;
    UINT br; // <--- Deve essere UINT, non WORD
    void (*user_app)(void) = (void*)SDRAM_BASE_ADDR;

    neorv32_uart0_puts("Searching BOOT.BIN... ");
    res = pf_open("BOOT.BIN");
    
    if (res != FR_OK) {
        neorv32_uart0_puts("NOT FOUND!\n");
        return;
    }
    neorv32_uart0_puts("Found!\n");

    // Incremento velocità SPI (100MHz / 4 = 25MHz) dopo il mount
    neorv32_spi_setup(CLK_PRSC_4, 0, 0, 0);

    neorv32_uart0_puts("Loading to SDRAM... ");
    
    // Lettura del file e scrittura diretta in SDRAM
    res = pf_read((void*)SDRAM_BASE_ADDR, MAX_LOAD_SIZE, &br);
    

    if (res == FR_OK && br > 0) {
        neorv32_uart0_puts("Done.\nLaunching Firmware...\n");
        neorv32_uart0_puts("---------------------------------\n");

        // Assicuriamoci che la UART abbia finito di trasmettere
        while (neorv32_uart0_tx_busy());
        
        // Sincronizzazione fondamentale (Fence) per la cache
        asm volatile ("fence.i");

        // JUMP!
        user_app();
        
    } else {
        neorv32_uart0_puts("ERROR: Read failed!\n");
    }
}

/*=======================================================================*/
/* Main Loop                                                             */
/*=======================================================================*/
int main (void) {
    // 1. Hardware Init
    if (neorv32_Init() != 0) return(1);

    // 2. Welcome Message
    OutputBootMessage();
    neorv32_gpio_port_set(0);
    
    // 3. SD Boot Sequence
    setup_sd();
    boot_from_sd();
    
    // 4. Fallback (se il boot fallisce, esegue il running light)
    term_printf("\r\nBoot failed or no SD. Starting fallback mode...\r\n");

    uint8_t value = 0;
    int i = 0;
    while (1) {
        neorv32_gpio_port_set(1 << i);
        i = (i + 1) % 8;
        neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 200);
    }   
    
    return(0);
}