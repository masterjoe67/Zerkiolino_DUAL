// i_input.c - Integrazione Tastiera PS/2 per Tiny_DSO_VGA (NEORV32)
// Joe & Zoe - 2026

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "doomtype.h"
#include "doomkeys.h"
#include "d_event.h"
#include "i_system.h"
#include "../PS2_Keyboard/ps2_kbd.h" // La tua libreria hardware

// Stato del tasto Shift per la digitazione (messaggi/salvataggi)
static int shiftdown = 0;
int vanilla_keyboard_mapping = 1;

// --- TABELLA DI TRADUZIONE SCANCODE SET 2 -> DOOM ---
// Mappa i byte che abbiamo visto (es. 0x66 per Backspace) verso Doom
static const unsigned char ps2_to_doom[256] = {
    [0x76] = KEY_ESCAPE,    [0x05] = KEY_F1,        [0x06] = KEY_F2,
    [0x04] = KEY_F3,        [0x0C] = KEY_F4,        [0x03] = KEY_F5,
    [0x0B] = KEY_F6,        [0x83] = KEY_F7,        [0x0A] = KEY_F8,
    [0x01] = KEY_F9,        [0x09] = KEY_F10,       [0x78] = KEY_F11,
    [0x07] = KEY_F12,       [0x0E] = '`',           [0x16] = '1',
    [0x1E] = '2',           [0x26] = '3',           [0x25] = '4',
    [0x2E] = '5',           [0x36] = '6',           [0x3D] = '7',
    [0x3E] = '8',           [0x46] = '9',           [0x45] = '0',
    [0x4E] = '-',           [0x55] = '=',           [0x66] = KEY_BACKSPACE,
    [0x0D] = KEY_TAB,       [0x15] = 'q',           [0x1D] = 'w',
    [0x24] = 'e',           [0x2D] = 'r',           [0x2C] = 't',
    [0x35] = 'y',           [0x3C] = 'u',           [0x43] = 'i',
    [0x44] = 'o',           [0x4D] = 'p',           [0x54] = '[',
    [0x5B] = ']',           [0x5D] = '\\',          [0x58] = KEY_CAPSLOCK,
    [0x1C] = 'a',           [0x1B] = 's',           [0x23] = 'd',
    [0x2B] = 'f',           [0x34] = 'g',           [0x33] = 'h',
    [0x3B] = 'j',           [0x42] = 'k',           [0x4B] = 'l',
    [0x4C] = ';',           [0x52] = '\'',          [0x5A] = KEY_ENTER,
    [0x12] = KEY_RSHIFT,    [0x1A] = 'z',           [0x22] = 'x',
    [0x21] = 'c',           [0x2A] = 'v',           [0x32] = 'b',
    [0x31] = 'n',           [0x3A] = 'm',           [0x41] = ',',
    [0x49] = '.',           [0x4A] = '/',           [0x59] = KEY_RSHIFT,
    [0x14] = KEY_FIRE,      [0x11] = KEY_LALT,       [0x29] = KEY_USE, // Space/USE
};

// --- GESTIONE TASTI ESTESI (Frecce, CTRL destro, ecc.) ---
static unsigned char TranslateExtended(uint8_t scancode) {
    switch(scancode) {
        case 0x75: return KEY_UPARROW;
        case 0x72: return KEY_DOWNARROW;
        case 0x6B: return KEY_LEFTARROW;
        case 0x74: return KEY_RIGHTARROW;
        case 0x14: return KEY_RCTRL; // Sparare
        case 0x11: return KEY_RALT;  // Strafe
        default:   return 0;
    }
}

// Trasformazione per tasti premuti con Shift
static const char shiftxform[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, ' ', '!', '"', '#', '$', '%', '&',
    '\"', '(', ')', '*', '+', '<', '_', '>', '?', ')', '!', '@', '#', '$', '%', 
    '^', '&', '*', '(', ':', ':', '<', '+', '>', '?', '@', 
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '!', ']', '"', '_',
    '\'', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '|', '}', '~', 127
};

static unsigned char GetTypedChar(unsigned char key) {
    if (shiftdown > 0 && key < sizeof(shiftxform)) {
        return shiftxform[key];
    }
    return key;
}

static void UpdateShiftStatus(int pressed, unsigned char key) {
    if (key == KEY_RSHIFT || key == KEY_RSHIFT) {
        if (pressed) shiftdown++;
        else shiftdown--;
        if (shiftdown < 0) shiftdown = 0;
    }
}

// --- CORE: LEGGE DALLA FIFO E INVIA A DOOM ---
void I_GetEvent(void) {
    event_t event;
    ps2_event_t ps2_ev;

    // Svuota la FIFO hardware del NEORV32
    while (ps2_get_event(&ps2_ev)) {
       // printf("EVENTO PS2 RILEVATO!\n"); // Se non leggi questo, il problema è a monte
        unsigned char doom_key = 0;

        // Traduzione
        if (ps2_ev.extended) {
            doom_key = TranslateExtended(ps2_ev.scancode);
        } else {
            doom_key = ps2_to_doom[ps2_ev.scancode];
        }

        // Se il tasto non è mappato, ignoralo
        if (doom_key == 0) continue;

        // Gestione Shift
        int is_pressed = (ps2_ev.state == PS2_KEY_PRESSED);
        UpdateShiftStatus(is_pressed, doom_key);

        // Costruzione evento
        event.type = is_pressed ? ev_keydown : ev_keyup;
        event.data1 = doom_key;
        event.data2 = is_pressed ? GetTypedChar(doom_key) : 0;
        event.data3 = 0;

        // Invia al motore
        D_PostEvent(&event);
    }
}

void I_InitInput(void) {
    // Inizializzazione eventuale (la FIFO hardware è già pronta al boot)
    shiftdown = 0;
}

// Implementazione minima per joystick/mouse se richiesti dal linker
//void I_StartTic (void) {}
//void I_UpdateNoBlit (void) {}