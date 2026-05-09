# =============================================================================
# Application Makefile - Progetto Zerkiolino (Firmware per SDRAM)
# =============================================================================

# Override the default CPU ISA
# Nota: Assicurati che il tuo hardware NEORV32 supporti Zfinx (FPU sui registri interi)
MARCH = rv32ia_zfinx_zicsr_zifencei

# Override default optimization goal
EFFORT = -Os

# Add extended debug symbols
USER_FLAGS += -ggdb -gdwarf-3

# -----------------------------------------------------------------------------
# CONFIGURAZIONE MEMORIA SDRAM (32MB Totali)
# -----------------------------------------------------------------------------

# 1. Indirizzo base della SDRAM (Sezione ROM del firmware)
USER_FLAGS += -Wl,--defsym,__neorv32_rom_base=0x90000000

# 2. Dimensione dedicata al CODICE (4MB sono più che sufficienti)
USER_FLAGS += -Wl,--defsym,__neorv32_rom_size=4M

# 3. Indirizzo base per i DATI/STACK (Offset di 4MB dall'inizio SDRAM)
USER_FLAGS += -Wl,--defsym,__neorv32_ram_base=0x90400000

# 4. Dimensione dedicata ai DATI (Restanti 28MB della SDRAM)
USER_FLAGS += -Wl,--defsym,__neorv32_ram_size=28M

# 5. Dimensione HEAP (Opzionale: decommenta se usi malloc)
# USER_FLAGS += -Wl,--defsym,__neorv32_heap_size=1M

# -----------------------------------------------------------------------------
# SORGENTI E INCLUDES
# -----------------------------------------------------------------------------

# Sorgenti dell'applicazione
# Nota: Ho mantenuto petit_fatfs se prevedi di leggere file anche dal firmware
APP_SRC = main.c inc/terminal.c inc/ee_printf.c petit_fatfs/pff.c petit_fatfs/diskio.c vga/vga.c vga/fonts/glcdfont.c

# Percorsi include
APP_INC += -I ./inc
APP_INC += -I ./Peripheral
APP_INC += -I ./petit_fatfs
APP_INC += -I ./vga
APP_INC += -I ./vga/fonts	

# -----------------------------------------------------------------------------
# NEORV32 SETUP
# -----------------------------------------------------------------------------

# Percorso della root directory di NEORV32
NEORV32_HOME ?= ../../neorv32

# Inclusione del makefile comune di NEORV32
include $(NEORV32_HOME)/sw/common/common.mk