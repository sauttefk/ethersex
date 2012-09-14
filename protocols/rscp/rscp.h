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

#define RSCP_CONFIG_VERSION   RSCP_EEPROM_START
#define RSCP_CONFIG_MAC0      (RSCP_CONFIG_VERSION + sizeof(uint16_t))
#define RSCP_CONFIG_MAC1      (RSCP_CONFIG_MAC0 + 1 * sizeof(uint8_t))
#define RSCP_CONFIG_MAC2      (RSCP_CONFIG_MAC0 + 2 * sizeof(uint8_t))
#define RSCP_CONFIG_MAC3      (RSCP_CONFIG_MAC0 + 3 * sizeof(uint8_t))
#define RSCP_CONFIG_MAC4      (RSCP_CONFIG_MAC0 + 4 * sizeof(uint8_t))
#define RSCP_CONFIG_MAC5      (RSCP_CONFIG_MAC0 + 5 * sizeof(uint8_t))
#define RSCP_CONFIG_CHANNEL_P (RSCP_CONFIG_MAC0 + 6 * sizeof(uint8_t))
#define RSCP_CONFIG_BUTTON_P  (RSCP_CONFIG_CHANNEL_P + sizeof(uint16_t))
#define RSCP_CONFIG_RULE_P    (RSCP_CONFIG_BUTTON_P + sizeof(uint16_t))
#define RSCP_CONFIG_NAME      (RSCP_CONFIG_RULE_P + sizeof(uint16_t))

uint16_t rscp_channel_p;
uint16_t rscp_button_p;
uint16_t rscp_rule_p;

struct rscp_eeprom_header
{
  uint16_t version;         // version number (currently 1)
  uint8_t  macaddress[6];   // mac address
  uint16_t channel_p;       // pointer to channel definitions (relative to start of configuration)
  uint16_t button_p;        // pointer to button definitions (relative to start of configuration)
  uint16_t rule_p;          // pointer to rule definitions (relative to start of configuration)
  uint8_t  name[];          // name of device (ASCII encoding, zero-terminated)
};


struct rscp_channel_definition
{
  uint16_t number;          // number of defined channels
};


struct rscp_channel_type_01 // binary input
{
  uint16_t channel_id;      // channel id
  uint8_t channel_type;     // channel type
  uint8_t port:4;           // port
  uint8_t pin:4;            // pin
  uint8_t negate:1;         // negate polarity
  uint8_t report:1;         // report change
  uint8_t pullup:1;         // weak pullup resistor
  uint8_t :5;               // unused
};


struct rscp_channel_type_02 // binary output
{
  uint16_t channel_id;      // channel id
  uint8_t channel_type;     // channel type
  uint8_t port:4;           // port
  uint8_t pin:4;            // pin
  uint8_t negate:1;         // negate polarity
  uint8_t report:1;         // report change
  uint8_t mode:2;           // output mode
  uint8_t :4;               // unused
};

struct rscp_button
{
  uint16_t button_id;       // button id
  uint16_t channel_id;      // associated channel id
  uint8_t report_press:1;   // report press of button
  uint8_t report_release:1; // report release of button
  uint8_t :6;               // unused
  uint16_t long_press;      // long press after N*20ms (0 = disabled)
  uint16_t repeat;          // repeat every N*20ms (0 = disabled)
};

#endif /* RSCP_SUPPORT */
#endif /* _RSCP_H */
