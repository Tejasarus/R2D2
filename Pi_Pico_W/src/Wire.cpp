/*
 * SPDX-License-Identifier: MIT
 * Converted from C++ TwoWire to C
 */

#include "Wire.h"
#include <i2c_fifo.h>
#include <assert.h>
#include <string.h>

// Forward declarations
struct TwoWire;
typedef void (*WireReceiveHandler)(size_t len);
typedef void (*WireRequestHandler)(void);

typedef enum {
    WIRE_MODE_UNASSIGNED = 0,
    WIRE_MODE_MASTER,
    WIRE_MODE_SLAVE
} wire_mode_t;

#define NO_ADDRESS 0xFF
#define WIRE_BUFFER_LENGTH 256

typedef struct TwoWire {
    wire_mode_t mode;
    uint8_t buf[WIRE_BUFFER_LENGTH];
    uint8_t bufLen;
    uint8_t bufPos;
    uint8_t txAddress;

    WireReceiveHandler receiveHandler;
    WireRequestHandler requestHandler;

    int index; // 0 for i2c0, 1 for i2c1
} TwoWire;

TwoWire Wire  = { .index = 0, .mode = WIRE_MODE_UNASSIGNED, .txAddress = NO_ADDRESS };
TwoWire Wire1 = { .index = 1, .mode = WIRE_MODE_UNASSIGNED, .txAddress = NO_ADDRESS };

//
// Helpers
//
static inline i2c_inst_t* two_wire_hw(TwoWire* wire) {
    return (wire->index == 0) ? i2c0 : i2c1;
}

//
// Public API
//

void two_wire_begin(TwoWire* wire) {
    assert(wire->txAddress == NO_ADDRESS);
    if (wire->mode != WIRE_MODE_UNASSIGNED) {
        i2c_slave_deinit(two_wire_hw(wire));
    }
    wire->mode = WIRE_MODE_MASTER;
    wire->bufLen = 0;
    wire->bufPos = 0;
}

void two_wire_begin_slave(TwoWire* wire, uint8_t selfAddress) {
    assert(wire->txAddress == NO_ADDRESS);
    if (wire->mode != WIRE_MODE_UNASSIGNED) {
        i2c_slave_deinit(two_wire_hw(wire));
    }
    wire->mode = WIRE_MODE_SLAVE;
    wire->bufLen = 0;
    wire->bufPos = 0;
    i2c_slave_init(two_wire_hw(wire), selfAddress, &two_wire_handle_event);
}

void two_wire_begin_transmission(TwoWire* wire, uint8_t address) {
    assert(wire->mode == WIRE_MODE_MASTER);
    assert(wire->txAddress == NO_ADDRESS);
    assert(address != NO_ADDRESS);

    wire->txAddress = address;
    wire->bufLen = 0;
    wire->bufPos = 0;
}

uint8_t two_wire_end_transmission(TwoWire* wire, bool sendStop) {
    assert(wire->mode == WIRE_MODE_MASTER);
    assert(wire->txAddress != NO_ADDRESS);
    assert(wire->bufPos == 0);

    int result = i2c_write_blocking(two_wire_hw(wire),
                                    wire->txAddress,
                                    wire->buf,
                                    wire->bufLen,
                                    !sendStop);

    wire->txAddress = NO_ADDRESS;
    uint8_t sentLen = wire->bufLen;
    wire->bufLen = 0;

    if (result < 0) {
        return 4; // error
    } else if (result < sentLen) {
        return 3; // NACK
    } else {
        return 0; // success
    }
}

uint8_t two_wire_request_from(TwoWire* wire, uint8_t address, size_t count, bool sendStop) {
    assert(wire->mode == WIRE_MODE_MASTER);
    assert(wire->txAddress == NO_ADDRESS);

    if (count > WIRE_BUFFER_LENGTH) count = WIRE_BUFFER_LENGTH;

    int result = i2c_read_blocking(two_wire_hw(wire),
                                   address,
                                   wire->buf,
                                   count,
                                   !sendStop);

    if (result < 0) {
        result = 0;
    }

    wire->bufLen = (uint8_t)result;
    wire->bufPos = 0;
    return (uint8_t)result;
}

size_t two_wire_write_byte(TwoWire* wire, uint8_t value) {
    assert(wire->mode != WIRE_MODE_UNASSIGNED);

    if (wire->mode == WIRE_MODE_MASTER) {
        assert(wire->txAddress != NO_ADDRESS);
        if (wire->bufLen == WIRE_BUFFER_LENGTH) {
            return 0;
        }
        wire->buf[wire->bufLen++] = value;
    } else {
        i2c_inst_t* i2c = two_wire_hw(wire);
        i2c_hw_t* hw = i2c_get_hw(i2c);
        while ((hw->status & I2C_IC_STATUS_TFNF_BITS) == 0) {
            tight_loop_contents();
        }
        i2c_write_byte(i2c, value);
    }
    return 1;
}

size_t two_wire_write(TwoWire* wire, const uint8_t* data, size_t size) {
    assert(wire->mode != WIRE_MODE_UNASSIGNED);

    if (wire->mode == WIRE_MODE_MASTER) {
        assert(wire->txAddress != NO_ADDRESS);
        if (size > (WIRE_BUFFER_LENGTH - wire->bufLen)) {
            size = WIRE_BUFFER_LENGTH - wire->bufLen;
        }
        memcpy(&wire->buf[wire->bufLen], data, size);
        wire->bufLen += size;
    } else {
        i2c_inst_t* i2c = two_wire_hw(wire);
        i2c_hw_t* hw = i2c_get_hw(i2c);
        for (size_t i = 0; i < size; i++) {
            while ((hw->status & I2C_IC_STATUS_TFNF_BITS) == 0) {
                tight_loop_contents();
            }
            i2c_write_byte(i2c, data[i]);
        }
    }
    return size;
}

void two_wire_on_receive(TwoWire* wire, WireReceiveHandler handler) {
    wire->receiveHandler = handler;
}

void two_wire_on_request(TwoWire* wire, WireRequestHandler handler) {
    wire->requestHandler = handler;
}

//
// Event handler (slave mode)
//
void __not_in_flash_func(two_wire_handle_event)(i2c_inst_t* i2c, i2c_slave_event_t event) {
    TwoWire* wire = (i2c_hw_index(i2c) == 0 ? &Wire : &Wire1);
    assert(wire->mode == WIRE_MODE_SLAVE);
    assert(wire->bufPos == 0);

    switch (event) {
        case I2C_SLAVE_RECEIVE: {
            for (size_t n = i2c_get_read_available(i2c); n > 0; n--) {
                uint8_t value = i2c_read_byte(i2c);
                if (wire->bufLen < WIRE_BUFFER_LENGTH) {
                    wire->buf[wire->bufLen++] = value;
                }
            }
            break;
        }
        case I2C_SLAVE_REQUEST:
            assert(wire->bufLen == 0);
            assert(wire->bufPos == 0);
            if (wire->requestHandler != NULL) {
                wire->requestHandler();
            }
            break;
        case I2C_SLAVE_FINISH:
            if (wire->bufLen > 0) {
                if (wire->receiveHandler != NULL) {
                    wire->receiveHandler(wire->bufLen);
                }
                wire->bufLen = 0;
                wire->bufPos = 0;
            }
            assert(wire->bufPos == 0);
            break;
        default:
            break;
    }
}
