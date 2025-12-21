#ifndef DFPLAYER_H
#define DFPLAYER_H

#include "pico/stdlib.h"
#include "hardware/uart.h"

// === CONFIG ===
// Adjust pins + UART instance to match your wiring
#define DFPLAYER_UART_ID uart1
#define DFPLAYER_TX_PIN  4   // Pico TX → DFPlayer RX
#define DFPLAYER_RX_PIN  5   // Pico RX ← DFPlayer TX
#define DFPLAYER_BAUDRATE 9600

// === API ===
void dfplayer_init(void);
void dfplayer_reset(void);
void dfplayer_set_volume(uint8_t vol);   // 0–30
void dfplayer_volume_up(void);
void dfplayer_volume_down(void);
void dfplayer_play(uint16_t track);      // play Nth track
void dfplayer_play_next(void);
void dfplayer_play_previous(void);
void dfplayer_stop(void);
void dfplayer_pause(void);
void dfplayer_resume(void);

#endif
