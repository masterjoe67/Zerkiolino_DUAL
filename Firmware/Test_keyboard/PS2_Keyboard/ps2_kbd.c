#include "ps2_kbd.h"

// Stati interni della decodifica scancode
static bool s_is_extended = false;
static bool s_is_released = false;

/**
 * @brief Verifica se ci sono byte pronti nella FIFO hardware
 */
bool ps2_has_data(void) {
    return (PS2_REG_STATUS & 0x01) ? true : false;
}

/**
 * @brief Estrae un evento completo dalla tastiera.
 * Gestisce i prefissi 0xE0 e 0xF0.
 * @return true se un evento valido (pressione o rilascio) è stato catturato.
 */
bool ps2_get_event(ps2_event_t *event) {
    while (ps2_has_data()) {
        uint8_t raw_byte = (uint8_t)(PS2_REG_DATA & 0xFF);

        // Caso 1: Prefisso Esteso
        if (raw_byte == 0xE0) {
            s_is_extended = true;
            continue; // Aspetta il prossimo byte
        }

        // Caso 2: Prefisso Rilascio (Break Code)
        if (raw_byte == 0xF0) {
            s_is_released = true;
            continue; // Aspetta il prossimo byte
        }

        // Caso 3: Scancode effettivo
        event->scancode = raw_byte;
        event->extended = s_is_extended;
        event->state    = s_is_released ? PS2_KEY_RELEASED : PS2_KEY_PRESSED;

        // Reset degli stati per il prossimo tasto
        s_is_extended = false;
        s_is_released = false;

        return true; // Evento pronto
    }

    return false; // Nessun evento completo disponibile
}