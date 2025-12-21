#include <stddef.h>
#include <string.h>
#include "i2c_slave.h"
#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <uni.h>

#include "sdkconfig.h"

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "dfplayer.h"

#define I2C_PORT i2c0
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
static const uint I2C_SLAVE_SDA_PIN = 8; 
static const uint I2C_SLAVE_SCL_PIN = 9; 
static const uint I2C_SLAVE_ADDRESS = 0x05;
static const uint I2C_BAUDRATE = 100000; 

#define LED_PIN 25
#define THRESHOLD 0.5

float prev_axis_x = 0;
float prev_axis_y = 0;

char button_input =' ';

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t* d);
bool button_pressed = false;

#define INPUT_A   (1u << 0)   // bit 0
#define INPUT_B   (1u << 1)   // bit 1
#define INPUT_X   (1u << 2)   // bit 2
#define INPUT_Y   (1u << 3)   // bit 3
#define INPUT_L   (1u << 4)   // bit 4
#define INPUT_R   (1u << 5)   // bit 5
#define INPUT_T   (1u << 6)   // bit 6
#define INPUT_Z   (1u << 7)   // bit 7
#define INPUT_U   (1u << 8)   // bit 8
#define INPUT_H   (1u << 9)   // bit 9
#define INPUT_J   (1u << 10)  // bit 10
#define INPUT_K   (1u << 11)  // bit 11
#define INPUT_E   (1u << 12)  // bit 12
#define INPUT_S   (1u << 13)  // bit 13
#define INPUT_D   (1u << 14)  // bit 14
#define INPUT_F   (1u << 15)  // bit 15
#define INPUT_C   (1u << 16)  // bit 16
#define INPUT_V   (1u << 17)  // bit 17
#define INPUT_N   (1u << 18)  // bit 18
#define INPUT_M   (1u << 19)  // bit 19

/*
Audio Key:
1 - Scream
2 - high pitched beep
3 - Fast Beeps
4 - classic r2 sound
5 - classic r2 sound 2

*/

volatile uint32_t  tx_data = 0;
volatile uint32_t last_button = 0;

static struct
{
    uint8_t mem[256];
    uint8_t mem_address;
    bool mem_address_written;
} context;

static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) 
    {
        case I2C_SLAVE_RECEIVE:
            i2c_read_byte_raw(i2c);
            break;

        case I2C_SLAVE_REQUEST: 
            i2c_write_byte(i2c, (uint8_t)(last_button & 0xFF));
            i2c_write_byte(i2c, (uint8_t)((last_button >> 8) & 0xFF));
            i2c_write_byte(i2c, (uint8_t)((last_button >> 16) & 0xFF));
            i2c_write_byte(i2c, (uint8_t)((last_button >> 24) & 0xFF));
            last_button = 0;
            break;

        case I2C_SLAVE_FINISH:
            context.mem_address_written = false;
            break;

        default:
            break;
    }
}

static void setup_slave() {
    gpio_init(I2C_SLAVE_SDA_PIN);
    gpio_set_function(I2C_SLAVE_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SDA_PIN);

    gpio_init(I2C_SLAVE_SCL_PIN);
    gpio_set_function(I2C_SLAVE_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SCL_PIN);

    i2c_init(i2c0, I2C_BAUDRATE);
    // configure I2C0 for slave mode
    i2c_slave_init(i2c0, I2C_SLAVE_ADDRESS, &i2c_slave_handler);
}

static void my_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("my_platform: init()\n");

    // Initialize LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Initialize I2C
    setup_slave();

    // Optionally delay for bus stability
    sleep_ms(100);

    srand(time(NULL));
    //Dfplayer setup
    dfplayer_init();
    sleep_ms(200);          // let UART settle
    dfplayer_reset();
    sleep_ms(500);          // DFPlayer needs ~500ms after reset
    dfplayer_set_volume(20);
}

static void my_platform_on_init_complete(void) {
    logi("my_platform: on_init_complete()\n");

    // Safe to call "unsafe" functions since they are called from BT thread

    // Start scanning and autoconnect to supported controllers.
    uni_bt_enable_new_connections_unsafe(true);

    // Based on runtime condition, you can delete or list the stored BT keys.
    if (1)
        uni_bt_del_keys_unsafe();
    else
        uni_bt_list_keys_unsafe();

    // Turn off LED once init is done.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    //    uni_bt_service_set_enabled(true);

    uni_property_dump_all();
}

static void my_platform_on_device_connected(uni_hid_device_t* d) {
    logi("my_platform: device connected: %p\n", d);
}

static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("my_platform: device disconnected: %p\n", d);
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    logi("my_platform: device ready: %p\n", d);

    // You can reject the connection by returning an error.
    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
   // static uint8_t leds = 0;
    //static uint8_t enabled = true;
    static uni_controller_t prev = {0};
    uni_gamepad_t* gp;

    prev = *ctl;
    // Print device Id before dumping gamepad.
    logi("(%p) id=%d ", d, uni_hid_device_get_idx_for_instance(d));
    uni_controller_dump(ctl);

    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            gp = &ctl->gamepad;
            tx_data = 0;  // reset each frame

            // Buttons
            if (gp->buttons & BUTTON_A)
            {
                tx_data |= INPUT_A; button_pressed = true; 

            }          
            if (gp->buttons & BUTTON_B)          tx_data |= INPUT_B; button_pressed = true;
            if (gp->buttons & BUTTON_X)          tx_data |= INPUT_X; button_pressed = true;
            if (gp->buttons & BUTTON_Y)          tx_data |= INPUT_Y; button_pressed = true;
            if (gp->buttons & BUTTON_SHOULDER_L) tx_data |= INPUT_L; button_pressed = true;
            if (gp->buttons & BUTTON_SHOULDER_R) tx_data |= INPUT_R; button_pressed = true; 
            if (gp->buttons & BUTTON_TRIGGER_R)  tx_data |= INPUT_T; button_pressed = true;
            if (gp->buttons & BUTTON_TRIGGER_L)  tx_data |= INPUT_Z; button_pressed = true;

            // D-Pad
            switch (gp->dpad) {
                case DPAD_UP:{
                    tx_data |= INPUT_U; 
                    button_pressed = true;
                    dfplayer_play((rand() % 19) + 1);
                    break;
                }
                    

                case DPAD_DOWN:{
                    tx_data |= INPUT_J; 
                    button_pressed = true;
                    dfplayer_play((rand() % 7) + 1);
                    break; 
                }
                    

                case DPAD_LEFT:  tx_data |= INPUT_H; button_pressed = true; break;
                case DPAD_RIGHT: tx_data |= INPUT_K; button_pressed = true; break;
            }

            // Left joystick
            if (gp->axis_x > THRESHOLD)   tx_data |= INPUT_F; button_pressed = true;  // Right
            if (gp->axis_x < -THRESHOLD)  tx_data |= INPUT_S; button_pressed = true;  // Left
            if (gp->axis_y < -THRESHOLD)  tx_data |= INPUT_E; button_pressed = true;  // Up
            if (gp->axis_y > THRESHOLD)   tx_data |= INPUT_D; button_pressed = true;  // Down

            // Right joystick (if you want to use them the same way)
            if (gp->axis_rx > THRESHOLD)       tx_data |= INPUT_M; button_pressed = true;  // Right
            if (gp->axis_rx < -THRESHOLD)      tx_data |= INPUT_V; button_pressed = true;  // Left
            if (gp->axis_ry < -THRESHOLD)      tx_data |= INPUT_C; button_pressed = true;  // Up
            if (gp->axis_ry > THRESHOLD)       tx_data |= INPUT_N; button_pressed = true;  // Down


            if(button_pressed)
            {
				last_button = tx_data;
                button_pressed = false;
            }
            break;
        default:
            loge("Unsupported controller class: %d\n", ctl->klass);
            break;
    }
}

static const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    switch (event) {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON:
            // Optional: do something when "system" button gets pressed.
            trigger_event_on_gamepad((uni_hid_device_t*)data);
            break;

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            // When the "bt scanning" is on / off. Could be triggered by different events
            // Useful to notify the user
            logi("my_platform_on_oob_event: Bluetooth enabled: %d\n", (bool)(data));
            break;

        default:
            logi("my_platform_on_oob_event: unsupported event: 0x%04x\n", event);
    }
}

//
// Helpers
//
static void trigger_event_on_gamepad(uni_hid_device_t* d) {
    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 50 /* duration ms */, 128 /* weak magnitude */,
                                          40 /* strong magnitude */);
    }

    if (d->report_parser.set_player_leds != NULL) {
        static uint8_t led = 0;
        led += 1;
        led &= 0xf;
        d->report_parser.set_player_leds(d, led);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        static uint8_t red = 0x10;
        static uint8_t green = 0x20;
        static uint8_t blue = 0x40;

        red += 0x10;
        green -= 0x20;
        blue += 0x40;
        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}

//
// Entry Point
//
struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {
        .name = "My Platform",
        .init = my_platform_init,
        .on_init_complete = my_platform_on_init_complete,
        //.on_device_discovered = my_platform_on_device_discovered,
        .on_device_connected = my_platform_on_device_connected,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_device_ready = my_platform_on_device_ready,
        .on_oob_event = my_platform_on_oob_event,
        .on_controller_data = my_platform_on_controller_data,
        .get_property = my_platform_get_property,
    };

    return &plat;
}