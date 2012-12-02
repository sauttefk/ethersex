/*
 * (c) 2012 Frank Sautter <ethersix@sautter.com>
 * (c) 2012 by Jörg Henne <hennejg@gmail.com>
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

#include <stdio.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdbool.h>

#include "core/eeprom.h"
#include "core/bool.h"
#include "protocols/uip/uip.h"
#include "protocols/uip/uip_router.h"
#include "services/clock/clock.h"
#include "hardware/ir/irmp/irmp.h"

#include "rscp_net.h"
#include "rscp_io.h"

#include "mtime.h"

#ifdef DEBUG_RSCP
#include "core/debug.h"
#define RSCP_DEBUG(str...) debug_printf ("RSCP: " str)
#else
#define RSCP_DEBUG(...)    ((void) 0)
#endif

#ifdef DEBUG_RSCP_CONF
#include "core/debug.h"
#define RSCP_DEBUG_CONF(str...) debug_printf ("RSCP-CONF: " str)
#else
#define RSCP_DEBUG_CONF(...)    ((void) 0)
#endif

uint8_t txidCounter;

void rscp_setup(void);
void rscp_main(void);
void rscp_handleMessage(rscp_nodeAddress *srcAddr, uint16_t msg_type,
    uint16_t payload_len, uint8_t * payload);
void rscp_init(void);
void rscp_periodic(void);
void rscp_sendPeriodicOutputEvents(void);
void rscp_sendPeriodicInputEvents(void);
void rscp_sendPeriodicIrmpEvents(void);

// *********************************************
// Message Types
#define RSCP_CHANNEL_EVENT            0x0001
#define RSCP_CHANNEL_REPORT           0x0002

#define RSCP_NODE_HEARTBEAT           0x0100
#define RSCP_SEGMENT_CTRL_HEARTBEAT   0x0101

#define RSCP_CHANNEL_STATE_CMD        0x8001

#define RSCP_FILE_TRANSFER_REQUEST    0x8110
#define RSCP_FILE_TRANSFER_RESPONSE   0x8111
#define RSCP_FILE_TRANSFER_DATA       0x8112
#define RSCP_FILE_TRANSFER_ACK        0x8113
#define RSCP_FILE_TRANSFER_ERROR      0x8114
// *********************************************

#define RSCP_FT_STATUS_DATA_FOLLOWS   0
#define RSCP_FT_STATUS_NOT_FOUND      1
#define RSCP_FT_STATUS_NOT_MODIFIED   2
#define RSCP_FT_STATUS_FAILED         3

#define RSCP_UNIT_COUNT               0x01  // Counter
#define RSCP_UNIT_VOLTAGE             0x02  // Voltage (V)
#define RSCP_UNIT_TEMPERATURE         0x03  // Temperatured (°C)
#define RSCP_UNIT_MASS                0x04  // Mass (kg)
#define RSCP_UNIT_TIME                0x05  // Time (s)
#define RSCP_UNIT_CURRENT             0x06  // Current (A)
#define RSCP_UNIT_VOLUME              0x07  // Volume (l)
#define RSCP_UNIT_PERCENT             0x08  // Percent
#define RSCP_UNIT_BOOLEAN             0x09  // Boolean (on/off)
#define RSCP_UNIT_SPEED               0x0a  // Speed (m/s)
#define RSCP_UNIT_ILLUMINATION        0x0b  // Illumination (Lux)

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

/*****************************************************************************
 * defines for eeprom config structure of rscp
 ****************************************************************************/

#define RSCP_EEPROM_START sizeof(struct eeprom_config_t)
#define RSCP_FREE_EEPROM (E2END - sizeof(struct eeprom_config_t) - 1)

#define rscpEE_byte(x, y, z) eeprom_read_byte((void *)(offsetof(x, y) + (void *)z))
#define rscpEE_word(x, y, z) eeprom_read_word((void *)(offsetof(x, y) + (void *)z))

#define rscpEEReadByte(x) eeprom_read_byte((void*)&(x))
#define rscpEEReadWord(x) eeprom_read_word((void*)&(x))
#define rscpEEReadDWord(x) eeprom_read_dword((void*)&(x))
#define rscpEEReadStruct(mem, x) eeprom_read_block(mem, (void*)(x), sizeof(*(x)))
#define rscpEEWriteByte(x, w) eeprom_write_byte((void*)&(x), w)
#define rscpEEWriteWord(x, w) eeprom_write_word((void*)&(x), w)
#define rscpEEWriteDWord(x, w) eeprom_write_dword((void*)&(x), w)

/**
 * header structure
 */

typedef struct __attribute__ ((packed))
{
  uint16_t version;         // version number (currently 1)
  uint8_t mac[6];           // mac address this config is meant for
  void * channel_p;         // pointer to channel definitions
  void * rule_p;            // pointer to rule definitions
  uint8_t name[];           // name of device (ASCII encoding, zero-terminated)
} rscp_conf_header;

typedef enum rscp_configStatus {
  rscp_noConfiguration = 0,
  rscp_configValid,
  rscp_configInvalid,
  rscp_configUpdating
} rscp_configStatus;

typedef struct __attribute__ ((packed))
{
  rscp_configStatus status;      // last known status of configuration
  uint16_t length;     // length of configuration data
  uint32_t crc32;      // the CRC32 (as used in ZIP/GZip) of the configuration data
  rscp_conf_header *p; // offset of configuration from start of EEPROM
} rscp_configuration;

rscp_configuration *rscpConfiguration;

#define NUM_SEGMENT_CONTROLLERS 4
typedef struct __attribute__ ((packed)) {
  enum {
    STOPPED = 0, RUNNING
  } state;
  rscp_nodeAddress address;
  mtime lastSeen;
} segmentController;
segmentController segmentControllers[NUM_SEGMENT_CONTROLLERS];

/**
 * channel structure
 */
enum
{
  RSCP_CHANNEL_BINARY_INPUT   = 0x01,
  RSCP_CHANNEL_BINARY_OUTPUT  = 0x02,
  RSCP_CHANNEL_COMPLEX_INPUT  = 0x11,
  RSCP_CHANNEL_COMPLEX_OUTPUT = 0x12,
  RSCP_CHANNEL_OWTEMPERATURE  = 0x30,
  RSCP_CHANNEL_ELTAKO_MS      = 0x31,
  RSCP_CHANNEL_DMX            = 0x32
};

typedef struct __attribute__ ((packed))
{
  uint8_t channelType;          // channel type
  uint16_t channel_list_p;      // channel list pointer
  uint16_t channel_list_items;  // number of channels of this type
  uint16_t firstChannelID;      // id of first channel of this type
} rscp_chList;

typedef struct __attribute__ ((packed))
{
  uint8_t numChannelTypes;
  rscp_chList channelTypes[];
} rscp_chConfig;

#define RSCP_CHANNEL_ID       offsetof(struct _rscp_conf_channel, channelId)
#define RSCP_CHANNEL_TYPE     offsetof(struct _rscp_conf_channel, channelType)

#endif /* RSCP_SUPPORT */
#endif /* _RSCP_H */
