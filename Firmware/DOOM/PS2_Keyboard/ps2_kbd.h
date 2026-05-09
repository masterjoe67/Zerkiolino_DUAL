#ifndef PS2_KBD_H
#define PS2_KBD_H

#include <stdint.h>
#include <stdbool.h>

// Indirizzo base definito nell'interconnessione Wishbone
#define PS2_BASE_ADDR 0x50000000

// Registri Hardware
#define PS2_REG_DATA   (*(volatile uint32_t*)(PS2_BASE_ADDR + 0))
#define PS2_REG_STATUS (*(volatile uint32_t*)(PS2_BASE_ADDR + 4))

// Tipi di evento
typedef enum {
    PS2_KEY_PRESSED,
    PS2_KEY_RELEASED
} ps2_state_t;

typedef struct {
    uint8_t scancode;
    ps2_state_t state;
    bool extended;
} ps2_event_t;

// Funzioni pubbliche
bool ps2_has_data(void);
bool ps2_get_event(ps2_event_t *event);

#endif // PS2_KBD_H