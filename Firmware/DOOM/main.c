#define __MAIN_C__

/*=======================================================================*/
/* Includes                                                              */
/*=======================================================================*/
#include "neorv32.h"
#include "terminal.h"
#include "vga/vga.h"
#include "neorv32_spi.h"
#include "petit_fatfs/pff.h"
#include "doomgeneric/doomgeneric_neorv32.h"

// Nota: se terminal.h non definisce term_printf, aggiungi:
// #define term_printf neorv32_uart0_printf

// Questa è la funzione di ingresso del motore Doom
extern void doomgeneric_Create(int argc, char **argv);

FATFS fs;

void setup_pmp_for_doom(void) {
    // 0x0F = (0x08 [TOR]) | (0x04 [X]) | (0x02 [W]) | (0x01 [R])
    uint8_t mode_tor_rwx = 0x0F; 

    // Regione 0: Tutto ciò che sta SOTTO la SDRAM (fino a 0x90000000)
    // Questo include l'IO e la tastiera a 0x50000000.
    neorv32_cpu_pmp_configure_region(0, 0x90000000, mode_tor_rwx); 

    // Regione 1: La SDRAM (da 0x90000000 a 0xA0000000)
    neorv32_cpu_pmp_configure_region(1, 0xA0000000, mode_tor_rwx); 

    neorv32_uart0_printf("PMP: Configurazione Manuale 0x0F applicata.\n");
}

/*=======================================================================*/
/* OutputBootMessage                                                     */
/*=======================================================================*/
static void OutputBootMessage (void)
{
   const char ResetScreen[] = { 0x1B, 'c', 0 };
   
   term_printf("%s", ResetScreen);
   // Uso la tua sintassi memorizzata
   neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 200);   

   term_printf("\r\n");
   term_printf("*********************************\r\n");
   term_printf("   Zerkiolino - SDRAM FW      \r\n"); // Cambiato per distinguerlo
   term_printf("*********************************\r\n");
   term_printf("Zoe: Salto in SDRAM riuscito!\r\n\n");
}

/*=======================================================================*/
/* neorv32_Init                                                          */
/*=======================================================================*/
static int neorv32_Init (void)
{
   term_Start();

   if (0 == neorv32_gpio_available()) 
   {
      neorv32_uart0_puts("\r\nError! No GPIO unit synthesized!\r\n");
      return(-1);
   }

   neorv32_rte_setup();
   return(0);   
}

FRESULT load_wad_to_sdram(const char* filename) {
    FRESULT res;
    UINT br;
    uint8_t buffer[512]; // Buffer più grande per velocità
    // Indirizzo base SDRAM 1 per il WAD (come definito nel tuo protocollo)
    uint32_t current_ptr = 0x91000000; 
    uint32_t total_written = 0;

    neorv32_uart0_printf("SD: Apertura %s... ", filename);
    res = pf_open(filename);
    
    if (res != FR_OK) {
        neorv32_uart0_printf("ERRORE! Code: %d\n", (uint32_t)res);
        return res;
    }
    neorv32_uart0_printf("OK!\n");

    neorv32_uart0_printf("SDRAM1: Caricamento in corso a 0x%x\n", current_ptr);

    vga_setTextColor(WHITE, 0x0000);
    vga_set_cursor(250, 380);
    vga_setTextSize(3);
    vga_Print("LOADING... ");
    vga_set_cursor(10, 420);
    vga_setTextSize(2);
    // Leggiamo finché c'è roba nel file (br > 0)
    while (1) {
        res = pf_read(buffer, sizeof(buffer), &br);
        
        if (res != FR_OK || br == 0) break;

        // Scrittura veloce in blocchi sulla SDRAM
        for (UINT i = 0; i < br; i++) {
            *(volatile uint8_t *)(current_ptr + i) = buffer[i];
        }

        current_ptr += br;
        total_written += (uint32_t)br;
        
        // Feedback ogni 64KB per non intasare la UART ma sapere che vive
        if ((total_written % 131072) == 0) {
            neorv32_uart0_printf("#"); 
            vga_Print("#");
        }
    }

    neorv32_uart0_printf("\nCompletato! Byte scritti: %u\n", total_written);
    return res;
}



/*=======================================================================*/
/* Main                                                                  */
/*=======================================================================*/
int main() {
    // 1. Inizializza il tuo hardware (UART, VGA, SDRAM)
   if (neorv32_Init() != 0) return(1);
   OutputBootMessage();
   setup_pmp_for_doom(); // Configura il PMP per Doom (SDRAM cacheable, IO bypass)
   neorv32_gpio_port_set(0);


    // Inizializza la VGA (se non già fatto nel bootloader)
   vga_set_write_page(0);
   vga_set_read_page(0);
   vga_setTextColor(RED, 0x0000);
   vga_setTextFont(1);
   vga_setTextSize(2);
   vga_set_cursor(10, 10);
   vga_clear(BLACK);

   vga_Print("Zerkiolino loadin WAD... ");
   vga_load_image_pfs("ZERK.BIN");

    // 2. Caricamento WAD (Esempio se lo hai in SDRAM)
   if (pf_mount(&fs) == FR_OK) {
        // 2. Carica il WAD in SDRAM 1
        load_wad_to_sdram("DOOM1.WAD");
    }
    
    vga_set_write_page(1);
    vga_clear(BLACK);
    vga_set_write_page(0);
    vga_clear(BLACK);
    // 3. Lancio di Doom
    // Passiamo degli argomenti "finti" per simulare la riga di comando
    char *argv[] = {"doom", "-iwad", "doom1.wad", "-window"};
    int argc = 4;

    // Zoe (Gemini) ti dice: "Si va in scena!"
   doomgeneric_Create(argc, argv);

    // Non dovrebbe mai arrivare qui
    while(1);
    return 0;
}


int main2 (void)
{
   uint8_t value;
   
   if (neorv32_Init() != 0) return(1);

   OutputBootMessage();
    
   neorv32_gpio_port_set(0);
   vga_set_write_page(0);
   vga_set_read_page(0);
   
   term_printf("Scotty! Energie, starting running light...\r\n");
   vga_setTextFont(1);
   vga_setTextSize(2);
   vga_setTextColor(RED, 0x0000);
   vga_set_cursor(0, 0);
   vga_clear(BLACK);
   vga_Print("Zerkiolino RISC-V VGA Test ");
   neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 1000);
   vga_load_image_pfs("ZERK.BIN");

   /*
    * Running light
    */   
   while (1) 
   {
      for (int i = 0; i < 8; i++)
      {
         value = (1<<i);
         neorv32_gpio_port_set(value & 0xFF);
         
         // Uso la tua sintassi memorizzata
         neorv32_aux_delay_ms(neorv32_sysinfo_get_clk(), 100);
      }
   }      
  
   return(0);
}                        