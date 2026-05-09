# Application makefile.
# Use this makefile to configure all relevant CPU / compiler options.

# Override the default CPU ISA
#MARCH = rv32ia_zicsr_zifencei
MARCH = rv32ia_zfinx_zicsr_zifencei

# Override the default RISC-V GCC prefix
#RISCV_PREFIX ?= riscv-none-elf-

# Override default optimization goal
EFFORT = -Os

# Add extended debug symbols
USER_FLAGS += -ggdb -gdwarf-3

# Adjust processor IMEM size (32*1024)
 USER_FLAGS += -Wl,--defsym,__neorv32_rom_size=32k

# Indirizzo base della tua SDRAM (mappata dal Mux Wishbone)
#USER_FLAGS += -Wl,--defsym,__neorv32_rom_base=0x90000000

# Dimensione della SDRAM (es. 32MB)
#USER_FLAGS += -Wl,--defsym,__neorv32_rom_size=32M

# Adjust processor DMEM size (16*1024)
USER_FLAGS += -Wl,--defsym,__neorv32_ram_size=16k

# Adjust maximum heap size
#USER_FLAGS += -Wl,--defsym,__neorv32_heap_size=3k

# Additional sources
#APP_SRC += $(wildcard ./*.c)
#APP_INC += -I .
APP_SRC = main.c inc/terminal.c  inc/ee_printf.c petit_fatfs/pff.c petit_fatfs/diskio.c  vga/fonts/glcdfont.c vga/vga.c PS2_Keyboard/ps2_kbd.c
APP_INC += -I ./inc


# Set path to NEORV32 root directory
NEORV32_HOME ?= ../../neorv32

# Include the main NEORV32 makefile
include $(NEORV32_HOME)/sw/common/common.mk
