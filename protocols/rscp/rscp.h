/*
 * (c) 2012 Frank Sautter <ethersix@sautter.com>
 *
 * This program is free software; you can redistsribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include "config.h"

#ifndef _RSCP_H
#define _RSCP_H

#ifdef RSCP_SUPPORT

#include <avr/io.h>
#include <avr/eeprom.h>

#include "rscp_net.h"
#include "core/eeprom.h"

#ifdef DEBUG_RSCP
#include "core/debug.h"
#define RSCP_DEBUG(str...) debug_printf ("rscp: " str)
#else
#define RSCP_DEBUG(...)    ((void) 0)
#endif

extern uint8_t rscp_mode;

void rscp_setup(void);
void rscp_main(void);
void rscp_get(uint8_t * src_addr, uint16_t msg_type, uint16_t payload_len,
              uint8_t * payload);

void rscp_init(void);
void rscp_periodic(void);
void rscp_sendHeartBeat(void);
void sendPeriodicOutputEvents(void);
void sendPeriodicInputEvents(void);
void sendPeriodicTemperature(void);


#define RSCP_SIZE_DEVURL              32
#define FIRMWARE_MAJOR_VERSION        0x00
#define FIRMWARE_MINOR_VERSION        0x00
#define FIRMWARE_SUB_MINOR_VERSION    0x01

#define RSCP_CHANNEL_EVENT            0x1001

#define RSCP_UNIT_COUNT               0x01  // Counter
#define RSCP_UNIT_VOLTAGE             0x02  // Voltage (V)
#define RSCP_UNIT_TEMPERATURE         0x03  // Temperatured (°C)
#define RSCP_UNIT_MASS                0x04  // Mass (kg)
#define RSCP_UNIT_TIME                0x05  // Time (s)
#define RSCP_UNIT_CURRENT             0x06  // Current (A)
#define RSCP_UNIT_VOLUME              0x07  // Volume (l)
#define RSCP_UNIT_PERCENT             0x08  // Percent
#define RSCP_UNIT_BOOLEAN             0x09  // Boolean (on/off)

#define RSCP_FIELD_CAT_LEN_TINY       0x0   // length encoded in the lll bits
#define RSCP_FIELD_CAT_LEN_BYTE       0x1   // length follows as one-byte length specifier
#define RSCP_FIELD_CAT_LEN_INT        0x2   // length follows as two-byte length specifier
#define RSCP_FIELD_CAT_LEN_IMMEDIATE  0x3   // immediate fields (type contains value)

#define RSCP_FIELD_TYPE_FALSE         0x0   // boolean false
#define RSCP_FIELD_TYPE_TRUE          0x1   // boolean true

#define RSCP_FIELD_TYPE_SIGNED        0x1   // signed
#define RSCP_FIELD_TYPE_UNSIGNED      0x2   // unsigned
#define RSCP_FIELD_TYPE_FLOAT         0x3   // float
#define RSCP_FIELD_TYPE_FIXED         0x4   // fixed
#define RSCP_FIELD_TYPE_STRING        0x5   // string iso8859-1
#define RSCP_FIELD_TYPE_ARRAY         0x6   // byte array

#define RSCP_FIELD_SUBTYPE_INT8       0x0 | RSCP_FIELD_TYPE_SIGNED << 3   // signed byte
#define RSCP_FIELD_SUBTYPE_INT16      0x1 | RSCP_FIELD_TYPE_SIGNED << 3   // signed int
#define RSCP_FIELD_SUBTYPE_INT32      0x2 | RSCP_FIELD_TYPE_SIGNED << 3   // signed long
#define RSCP_FIELD_SUBTYPE_INT64      0x3 | RSCP_FIELD_TYPE_SIGNED << 3   // signed long long

#define RSCP_FIELD_SUBTYPE_UINT8      0x0 | RSCP_FIELD_TYPE_UNSIGNED << 3 // unsigned byte
#define RSCP_FIELD_SUBTYPE_UINT16     0x1 | RSCP_FIELD_TYPE_UNSIGNED << 3 // unsigned int
#define RSCP_FIELD_SUBTYPE_UINT32     0x2 | RSCP_FIELD_TYPE_UNSIGNED << 3 // unsigned long
#define RSCP_FIELD_SUBTYPE_UINT64     0x3 | RSCP_FIELD_TYPE_UNSIGNED << 3 // unsigned long long

#define RSCP_FIELD_SUBTYPE_FLOAT      0x0 | RSCP_FIELD_TYPE_FLOAT << 3    // unsigned float
#define RSCP_FIELD_SUBTYPE_DOUBLE     0x1 | RSCP_FIELD_TYPE_FLOAT << 3    // unsigned double

#define RSCP_FIELD_SUBTYPE_FIXED8     0x0 | RSCP_FIELD_TYPE_FIXED << 3    // fixed decimal 2 bits exponent; 6bit value
#define RSCP_FIELD_SUBTYPE_FIXED16    0x1 | RSCP_FIELD_TYPE_FIXED << 3    // fixed decimal 3 bits exponent; 13bit value
#define RSCP_FIELD_SUBTYPE_FIXED32    0x2 | RSCP_FIELD_TYPE_FIXED << 3    // fixed decimal 4 bits exponent; 28bit value


/* actions */
#define RSCP_ACTION_QUIT              0x0 // end of action table reached
#define RSCP_ACTION_PASSTHROUGH       0x1 // reflect state of input
#define RSCP_ACTION_ALWAYS_ON         0x2 // output is always on
#define RSCP_ACTION_ALWAYS_OFF        0x3 // output is always off
#define RSCP_ACTION_TOGGLE            0x4 // toggle light
#define RSCP_ACTION_TOGGLE_DUAL       0x5 // toggle two lights
#define RSCP_ACTION_TWO_STAGE         0x6 // two stage light

#define RSCP_ACTION_RETRIGGER_TIMER   0x8 // retriggerable timer
#define RSCP_ACTION_BLINK             0x9 // blinker

#define RSCP_ACTION_AWNING            0xd // awning
#define RSCP_ACTION_BLINDS            0xe // blinds
#define RSCP_ACTION_WINDOW            0xf // windows

/**
 *  eeprom action table
 *
 * ??? mask for input event (mac/type)
 * ??? how to match input channel / temperature
 *
 * uint8    : type of action
 * uint8    : length of action item
 * uint8[6] : mac-address of sender
 * uint16   : type of event
 * uint8    : output 1
 * uint16   : delay
 * uint8    : output 2
*/

#define RSCP_EEPROM_START sizeof(struct eeprom_config_t)
#define RSCP_FREE_EEPROM (E2END - sizeof(struct eeprom_config_t) - 1)

/*****************************************************************************
 * defines for eeprom config structure of rscp
 ****************************************************************************/

/**
 * header structure
 */

typedef struct __attribute__ ((packed))  __rscp_conf_header
{
  uint16_t version;       // version number (currently 1)
  uint8_t mac[6];         // mac address this config is meant for
  uint16_t channel_items; // number of channel items
  uint16_t channel_p;     // pointer to channel definitions
  uint16_t button_items;  // number of button items
  uint16_t button_p;      // pointer to button definitions
  uint16_t rule_items;    // number of rule items
  uint16_t rule_p;        // pointer to rule definitions
  uint8_t name[];         // name of device (ASCII encoding, zero-terminated)
} rscp_conf_header;


uint16_t rscp_channel_items;
uint16_t rscp_channel_p;
uint16_t rscp_button_items;
uint16_t rscp_button_p;
uint16_t rscp_rule_items;
uint16_t rscp_rule_p;

/**
 * channel structure
 */
typedef struct __attribute__ ((packed))  _rscp_conf_channel
{
  uint16_t channelId;
  uint8_t channelType;
  union {
    struct {
      uint16_t port;            // port id
      union {
        uint8_t flags;          // bit flags
        uint8_t negate:1;       // negate polarity
        uint8_t report:1;       // report change
        uint8_t pullup:1;       // weak pullup resistor
        uint8_t debounceDelay:4;// the debounce delay in increments of 20ms
        uint8_t :1;             // unused
      };
    } chType01;                 // channel type 0x01 (binary input)
    struct {
      uint16_t port;            // port id
      union {
        uint8_t flags;          // bit flags
        uint8_t negate:1;       // negate polarity
        uint8_t report:1;       // report change
        uint8_t mode:2;         // output mode
        uint8_t :4;             // unused
      };
    } chType02;                 // channel type 0x02 (binary outnput)
    struct {
      uint8_t flags;            // flags (currently unused)
      uint8_t numports;         // number of channels to follow
      uint8_t numstates;        // number of states to follow
      uint8_t ports_states[];
    } chType11;                 // channel type 0x11 (complex input)
    struct {
      uint8_t flags;            // flags (currently unused)
      uint8_t numports;         // number of channels to follow
      uint8_t numstates;        // number of states to follow
    } chType12;                 // channel type 0x12 (complex output)
    struct {
      uint8_t owROM[8];         // onewire ROM code
      uint16_t interval;        // report-interval (s)
      int16_t tempHi;           // report-above (°C / 10)
      int16_t tempLo;           // report-below (°C / 10)
    } chType30;                 //  channel type 0x30 (temperature)
  };
} rscp_conf_channel;

typedef struct  __attribute__ ((packed))
{
	uint16_t port;            // port id
  union {
    uint8_t flags;          // bit flags
    struct {
      uint8_t :1;
      uint8_t debounceDelay:4;// the debounce delay in increments of 20ms
      uint8_t pullup:1;       // weak pullup resistor
      uint8_t report:1;       // report change
      uint8_t negate:1;       // negate polarity
    };
  };
  uint8_t debounceCounter:4;
	uint8_t lastRawState:1;
	uint8_t lastDebouncedState:1;
	uint8_t didChangeState:1;
  uint8_t :1;
} rscp_binaryInputChannel;

uint16_t rscp_numBinaryInputChannels;
rscp_binaryInputChannel *rscp_binaryInputChannels;

//typedef struct  __attribute__ ((packed))
//{
//	rscp_binary_input_channel **channels; // pointers to used channels
//	union {
//    uint8_t flags;          // bit flags
//    uint8_t reportPress:1;  // report button press
//    uint8_t reportRelease:1;// report button release
//	};
//	uint16_t longPressDelay;
//	uint16_t repeatInterval;
//} rscp_conf_complex_input_channel;
//
//typedef struct  __attribute__ ((packed))
//{
//	rscp_binary_input_channel* bic;         // the channel id used by the button
//	union {
//    uint8_t flags;          // bit flags
//    uint8_t reportPress:1;  // report button press
//    uint8_t reportRelease:1;// report button release
//	};
//	uint16_t longPressDelay;
//	uint16_t repeatInterval;
//} rscp_conf_button;

#define RSCP_CHT11_PORTID       0
#define RSCP_CHT11_PORT_FLAGS   (RSCP_CHT11_PORTID + sizeof(uint16_t))
// size of one channel id
#define RSCP_CHT11_PORT_SIZE    (RSCP_CHT11_PORT_FLAGS + sizeof(uint8_t))


#define RSCP_CHT11_PORTSTATES   0
// size of one portstate
#define RSCP_CHT11_STATE_SIZE    sizeof(uint8_t)


/**
 * channel type 0x12 (complex output)
 */

// port id
#define RSCP_CHT12_PORTID       0
// port flags
#define RSCP_CHT12_PORT_FLAGS   (RSCP_CHT12_PORTID + sizeof(uint16_t))
// size of one channel id
#define RSCP_CHT12_PORT_SIZE    (RSCP_CHT12_PORT_FLAGS + sizeof(uint8_t))

// port states
#define RSCP_CHT12_PORTSTATES   0
// size of one portstate
#define RSCP_CHT12_STATE_SIZE    sizeof(uint8_t)


#define rscpEE_byte(x, y, z) eeprom_read_byte((void *)(offsetof(x, y) + z))
#define rscpEE_word(x, y, z) eeprom_read_word((void *)(offsetof(x, y) + z))


#define RSCP_CHANNEL_ID         0
#define RSCP_CHANNEL_TYPE       (RSCP_CHANNEL_ID + sizeof(uint16_t))

#define RSCP_CHANNEL_BINARY_INPUT   0x01
#define RSCP_CHANNEL_BINARY_OUTPUT  0x02
#define RSCP_CHANNEL_COMPLEX_INPUT  0x11
#define RSCP_CHANNEL_COMPLEX_OUTPUT 0x12
#define RSCP_CHANNEL_TEMPERATURE    0x30

#define RSCP_CHT01_SIZE         6
#define RSCP_CHT02_SIZE         6
#define RSCP_CHT11_HEADSIZE     6
#define RSCP_CHT12_HEADSIZE     6
#define RSCP_CHT30_SIZE         17

/**
 * button
 */
// button id
#define RSCP_BUTTON_ID          0
// associated channel id
#define RSCP_BUTTON_CHANNEL     (RSCP_BUTTON_ID + sizeof(uint16_t))
// flags
#define RSCP_BUTTON_FLAGS       (RSCP_BUTTON_CHANNEL + sizeof(uint16_t))
// long press after N*20 ms (0 = disabled)
#define RSCP_BUTTON_LONGPRESS   (RSCP_BUTTON_FLAGS + sizeof(uint8_t))
// repeat every N*20 ms (0 = disabled)
#define RSCP_BUTTON_REPEAT      (RSCP_BUTTON_LONGPRESS + sizeof(uint16_t))
// size of button structure
#define RSCP_BUTTON_SIZE        (RSCP_BUTTON_REPEAT + sizeof(uint16_t))

//typedef struct rscp_button_flags
//{
//  uint8_t report_press:1;   // report press of button
//  uint8_t report_release:1; // report release of button
//  uint8_t :6;               // unused
//};

#endif /* RSCP_SUPPORT */
#endif /* _RSCP_H */
