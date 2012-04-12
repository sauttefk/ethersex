/*
 * Copyright (c) 2012 by Frank Sautter <ethersix@sautter.com>
 *
 * This program is free software; you can redistribute it and/or modify it
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

#ifndef _VSCP_NET_H
#define _VSCP_NET_H

/* constants */
#define VSCP_MODE_CAN                           0
#define VSCP_MODE_RS232                         1
#define VSCP_MODE_RS485                         2
#define VSCP_MODE_RAWETHERNET                   3
#define VSCP_MODE_TCP                           4
#define VSCP_MODE_UDP                           5
#define VSCP_MODE_MIWI                          6


#define VSCP_DM_MATRIX_BASE                     0x200 //FIXME: use top of eeprom pointer

#define VSCP_MAJOR_VERSION                      1
#define VSCP_MINOR_VERSION                      5

#define VSCP_FIRMWARE_MAJOR_VERSION             0x00
#define VSCP_FIRMWARE_MINOR_VERSION             0x00
#define VSCP_FIRMWARE_SUB_MINOR_VERSION         0x01

#define VSCP_RAWETHERNET_VERSION                0

// State machine states
#define VSCP_STATE_STARTUP                      0x00  // Cold/warm reset
#define VSCP_STATE_INIT                         0x01  // Assigning nickname
#define VSCP_STATE_PREACTIVE                    0x02  // Waiting for host initialixation
#define VSCP_STATE_ACTIVE                       0x03  // The normal state
#define VSCP_STATE_ERROR                        0x04  // error state. Big problems.

// State machine sub states
#define VSCP_SUBSTATE_NONE                      0x00  // No state


#define VSCP_ETHTYPE   9598         // 0x257E
#define VSCP_UDPH_LEN  1            // size of upd header
#define VSCP_RAWH_LEN  14           // size of raw ethernet header
#define VSCP_CTGS_LEN  22           // size of class, type, guid and size
#define VSCP_CRC_LEN   2            // size of crc

#define VSCP_UDP_POS_HEAD                       0
#define VSCP_UDP_POS_CLASS                      1
#define VSCP_UDP_POS_TYPE                       3
#define VSCP_UDP_POS_GUID                       5
#define VSCP_UDP_POS_SIZE                       21
#define VSCP_UDP_POS_DATA                       23

#define VSCP_RAW_POS_VERSION                    0
#define VSCP_RAW_POS_HEAD                       1
#define VSCP_RAW_POS_SUBSOURCE                  5
#define VSCP_RAW_POS_TIMESTAMP                  7
#define VSCP_RAW_POS_OBID                       11
#define VSCP_RAW_POS_CLASS                      15
#define VSCP_RAW_POS_TYPE                       17
#define VSCP_RAW_POS_SIZE                       19
#define VSCP_RAW_POS_DATA                       21

#define VSCP_LEVEL2_PRIORITY_0                  0x00000000
#define VSCP_LEVEL2_PRIORITY_1                  0x20000000
#define VSCP_LEVEL2_PRIORITY_2                  0x40000000
#define VSCP_LEVEL2_PRIORITY_3                  0x60000000
#define VSCP_LEVEL2_PRIORITY_4                  0x80000000
#define VSCP_LEVEL2_PRIORITY_5                  0xA0000000
#define VSCP_LEVEL2_PRIORITY_6                  0xC0000000
#define VSCP_LEVEL2_PRIORITY_7                  0xE0000000

#define VSCP_LEVEL2_PRIORITY_HIGH               VSCP_LEVEL2_PRIORITY_0
#define VSCP_LEVEL2_PRIORITY_MEDIUM             VSCP_LEVEL2_PRIORITY_4
#define VSCP_LEVEL2_PRIORITY_LOW                VSCP_LEVEL2_PRIORITY_7

#define VSCP_LEVEL2_MASK_PRIORITY               0xE0000000
#define VSCP_LEVEL2_MASK_CRYPTO                 0x1E000000

#define VSCP_LEVEL1_MAXDATA                     8
#define VSCP_LEVEL2_MAXDATA                     (512 - 25)


//* * * Data Coding for VSCP packets * * *

// data format masks
#define VSCP_MASK_DATACODING_TYPE               0xE0
#define VSCP_MASK_DATACODING_UNIT               0x18
#define VSCP_MASK_DATACODING_INDEX              0x03

// data coding types
#define VSCP_DATACODING_BIT                     0x00
#define VSCP_DATACODING_BYTE                    0x20
#define VSCP_DATACODING_STRING                  0x40
#define VSCP_DATACODING_INTEGER                 0x60
#define VSCP_DATACODING_NORMALIZED              0x80
#define VSCP_DATACODING_SINGLE                  0xA0
#define VSCP_DATACODING_RESERVED1               0xC0
#define VSCP_DATACODING_RESERVED2               0xE0

// data coding index of value
#define VSCP_DATACODING_INDEX0                  0x00
#define VSCP_DATACODING_INDEX1                  0x01
#define VSCP_DATACODING_INDEX2                  0x02
#define VSCP_DATACODING_INDEX3                  0x03
#define VSCP_DATACODING_INDEX4                  0x04
#define VSCP_DATACODING_INDEX5                  0x05
#define VSCP_DATACODING_INDEX6                  0x06
#define VSCP_DATACODING_INDEX7                  0x07


#ifndef htonl
#define htonl(x) __builtin_bswap32(x)
#endif /* !htonl */
#ifndef ntohl
#define ntohl htonl
#endif /* !ntohl */


/* structs */
struct vscp_udp_event
{
  uint8_t  head;        // bit 765:  prioriy 0-7 where 0 is highest
                        // bit 4:    hardcoded, true for a hardcoded device
                        // bit 3210: reserved
  uint16_t class;       // VSCP class
  uint16_t type;        // VSCP type
  uint8_t  guid[16];    // originating node address MSB(0) -> LSB(15)
  uint16_t size;        // number of valid data bytes
  uint8_t  data[VSCP_LEVEL2_MAXDATA];  // data; max 487 (512- 25) bytes
};


struct vscp_raw_event
{
  uint8_t  version;     // version; currently 0
  uint32_t head;        // bit 31-29: prioriy 0-7 where 0 is highest
                        // bit 28-25: cryptographic algorithm
                        // bit 24-00: reserved
  uint16_t subsource;   // subunit of an ethernet device
                        // last two bytes of the GUID
  uint32_t timestamp;   // timestamp in microseconds
  uint16_t class;       // VSCP class
  uint16_t type;        // VSCP type
  uint16_t size;        // number of valid data bytes
  uint8_t  data[VSCP_LEVEL2_MAXDATA];  // data; max 487 (512- 25) bytes
};


/* prototypes */
void vscp_net_init(void);
void vscp_net_udp(void);
void vscp_net_raw(void);

#endif /* _VSCP_NET_H */

