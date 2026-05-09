#define __MAIN_C__

/*=======================================================================*/
/* Includes                                                              */
/*=======================================================================*/
#include "neorv32.h"
#include "terminal.h"
#include "vga/vga.h"
#include "neorv32_spi.h"
#include "petit_fatfs/pff.h"

// Nota: se terminal.h non definisce term_printf, aggiungi:
// #define term_printf neorv32_uart0_printf

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

/*=======================================================================*/
/* Main                                                                  */
/*=======================================================================*/
int main (void)
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