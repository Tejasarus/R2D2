#include "dfplayer.h"

static void dfplayer_send_cmd(uint8_t cmd, uint16_t param) {
    uint8_t buf[10];

    buf[0] = 0x7E;    // start
    buf[1] = 0xFF;    // version
    buf[2] = 0x06;    // length
    buf[3] = cmd;     // command
    buf[4] = 0x01;    // feedback: 1=request, 0=ignore
    buf[5] = (param >> 8) & 0xFF;
    buf[6] = param & 0xFF;

    uint16_t checksum = 0 - (buf[1] + buf[2] + buf[3] + buf[4] + buf[5] + buf[6]);
    buf[7] = (checksum >> 8) & 0xFF;
    buf[8] = checksum & 0xFF;

    buf[9] = 0xEF;    // end

    uart_write_blocking(DFPLAYER_UART_ID, buf, sizeof(buf));
}

void dfplayer_init(void) {
    uart_init(DFPLAYER_UART_ID, DFPLAYER_BAUDRATE);
    gpio_set_function(DFPLAYER_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(DFPLAYER_RX_PIN, GPIO_FUNC_UART);
}

void dfplayer_reset(void) {
    dfplayer_send_cmd(0x0C, 0);
    sleep_ms(200);
}

void dfplayer_set_volume(uint8_t vol) {
    if (vol > 30) vol = 30;
    dfplayer_send_cmd(0x06, vol);
}

void dfplayer_volume_up(void) {
    dfplayer_send_cmd(0x04, 0);
}

void dfplayer_volume_down(void) {
    dfplayer_send_cmd(0x05, 0);
}

void dfplayer_play(uint16_t track) {
    dfplayer_send_cmd(0x03, track);
}

void dfplayer_play_next(void) {
    dfplayer_send_cmd(0x01, 0);
}

void dfplayer_play_previous(void) {
    dfplayer_send_cmd(0x02, 0);
}

void dfplayer_stop(void) {
    dfplayer_send_cmd(0x16, 0);
}

void dfplayer_pause(void) {
    dfplayer_send_cmd(0x0E, 0);
}

void dfplayer_resume(void) {
    dfplayer_send_cmd(0x0D, 0);
}
