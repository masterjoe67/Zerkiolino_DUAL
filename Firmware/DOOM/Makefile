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
USER_FLAGS += -Wl,--defsym,__neorv32_heap_size=8M

USER_FLAGS += -mno-strict-align

# -----------------------------------------------------------------------------
# SORGENTI E INCLUDES
# -----------------------------------------------------------------------------
\
# Sorgenti dell'applicazione
# Nota: Ho mantenuto petit_fatfs se prevedi di leggere file anche dal firmware
APP_SRC = main.c inc/terminal.c inc/ee_printf.c petit_fatfs/pff.c petit_fatfs/diskio.c vga/vga.c vga/fonts/glcdfont.c PS2_Keyboard/ps2_kbd.c


APP_SRC += doomgeneric/doomgeneric.c d_loop.c m_cheat.c i_video.c v_video.c i_sound.c m_controls.c i_timer.c i_input.c i_joystick.c
APP_SRC += doomgeneric/doomgeneric_neorv32.c tables.c p_switch.c p_plats.c sounds.c d_iwad.c m_menu.c sha1.c
APP_SRC +=  d_net.c d_items.c g_game.c m_menu.c m_misc.c p_doors.c w_file.c statdump.c g_game.c\
			m_argv.c p_floor.c p_telept.c info.c p_sight.c m_bbox.c p_saveg.c d_net.c w_checksum.c\
           p_tick.c p_setup.c p_user.c r_main.c w_wad.c z_zone.c m_fixed.c m_config.c\
           am_map.c hu_lib.c hu_stuff.c  s_sound.c v_video.c m_random.c i_endoom.c\
           f_finale.c f_wipe.c d_event.c d_mode.c i_system.c i_video.c \
           st_lib.c st_stuff.c p_map.c p_maputl.c doomstat.c p_lights.c\
           p_inter.c p_mobj.c p_enemy.c p_pspr.c p_spec.c r_bsp.c \
           r_data.c r_draw.c r_plane.c r_segs.c r_sky.c r_things.c w_main.c \
           w_wad.c wi_stuff.c p_ceilng.c w_file_stdc.c w_file.c dstrings.c d_main.c dummy.c

# Percorsi include
APP_INC += -I ./inc
APP_INC += -I ./Peripheral
APP_INC += -I ./petit_fatfs
APP_INC += -I ./vga
APP_INC += -I ./vga/fonts	
APP_INC += -I ./doomgeneric
# -----------------------------------------------------------------------------
# NEORV32 SETUP
# -----------------------------------------------------------------------------

# Percorso della root directory di NEORV32
NEORV32_HOME ?= ../../neorv32

# Inclusione del makefile comune di NEORV32
include $(NEORV32_HOME)/sw/common/common.mk