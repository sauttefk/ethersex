/*
 * (c) 2012 by Frank Sautter <ethersix@sautter.com>
 * (c) 2012 by Jörg Henne <hennejg@gmail.com>
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

#include "config.h"

#ifndef _RSCP_NET_H
#define _RSCP_NET_H

#ifdef DEBUG_RSCP_NET
#include "core/debug.h"
#define RSCP_DEBUG_NET(str...) debug_printf ("RSCP-NET: " str)
#else
#define RSCP_DEBUG_NET(...)    ((void) 0)
#endif

#ifndef htonl
#define htonl(x) __builtin_bswap32(x)
#endif /* htonl */
#ifndef ntohl
#define ntohl htonl
#endif /* ntohl */

/* constants */
#define RSCP_MESSAGE_VERSION          0

typedef enum rscp_networkMode {
  rscp_ModeRawEthernet,
  rscp_ModeUDP,
  rscp_ModeCAN,
} rscp_networkMode_t;

/* structs */
typedef struct rscp_message
{
  uint8_t  version;                   // The version, currently 1
  uint8_t  header_len;                // The length of the header
  uint32_t timestamp;                 // timestamp in miliseconds
  uint16_t msg_type;                  // RSCP message type
  uint16_t payload_len;               // number of valid data bytes
  uint8_t  payload[512];              // data; max 512 bytes
} rscp_message_t;

typedef struct rscp_udp_message
{
  struct uip_eth_addr mac;            // mac address of sender
  rscp_message_t message;             // rscp message
} rscp_udp_message_t;

typedef struct rscp_payloadBuffer {
  uint8_t  *start;
  uint8_t  *pos;
} rscp_payloadBuffer_t;

typedef struct rscp_ipNodeAddress {
  uip_ipaddr_t ipAddress;
} rscp_ipNodeAddress;

typedef struct rscp_ethNodeAddress {
  struct uip_eth_addr macAddress;
} rscp_ethNodeAddress;

typedef struct rscp_canNodeAddress {
  uint8_t canAddress;
} rscp_canNodeAddress;

typedef struct rscp_nodeAddress {
  rscp_networkMode_t type;
  union u {
    rscp_ipNodeAddress ipNodeAddress;
    rscp_ethNodeAddress ethNodeAddress;
    rscp_canNodeAddress canNodeAddress;
  } u;
} rscp_nodeAddress;

#define RSCP_RAWH_LEN                 sizeof(struct uip_eth_hdr)
#define RSCP_HEADER_LEN               offsetof(rscp_message_t, payload)
#define RSCP_UDP_HEADER_LEN           offsetof(rscp_udp_message_t, message.payload)

/* prototypes */
void rscp_net_init(void);
void rscp_net_raw(void);
rscp_payloadBuffer_t* rscp_getPayloadBuffer();
void rscp_transmit(uint16_t messageType, rscp_nodeAddress *dst);

/*
 * Field types
 */
typedef enum {
  rscp_field_Byte = 0x01,
  rscp_field_UnsignedByte = 0x02,
  rscp_field_Short = 0x03,
  rscp_field_UnsignedShort =  0x04,
  rscp_field_Integer = 0x05,
  rscp_field_UnsignedInteger =  0x06,
  rscp_field_Long = 0x07,
  rscp_field_UnsignedLong =  0x08,
  rscp_field_Float = 0x09,
  rscp_field_Double = 0x0a,
  rscp_field_Decimal16 = 0x0b,
  rscp_field_Decimal24 = 0x0c,
  rscp_field_Decimal32 = 0x0d,
  rscp_field_BooleanFalse = 0x10,
  rscp_field_BooleanTrue = 0x11
} rscp_fieldType;

int8_t rscp_encodeChannel(uint16_t channel, rscp_payloadBuffer_t *buffer);

int8_t rscp_encodeBooleanField(int8_t value, rscp_payloadBuffer_t *buffer);

int8_t rscp_encodeUInt8Field(uint8_t value, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeUInt16Field(uint16_t value, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeUInt32Field(uint32_t value, rscp_payloadBuffer_t *buffer);

int8_t rscp_encodeInt8Field(int8_t value, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeInt16Field(int16_t value, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeInt32Field(int32_t value, rscp_payloadBuffer_t *buffer);

int8_t rscp_encodeUInt8 (uint8_t value, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeUInt16 (uint16_t value, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeUInt32 (uint32_t value, rscp_payloadBuffer_t *buffer);

int8_t rscp_encodeInt8 (int8_t value, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeInt16 (int16_t value, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeInt32 (int32_t value, rscp_payloadBuffer_t *buffer);

int8_t rscp_encodeRaw(void *data, uint16_t length, rscp_payloadBuffer_t *buffer);

int8_t rscp_encodeDecimal16Field(int16_t significand, int8_t scale,
    rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeDecimal24Field(int32_t significand, int8_t scale,
    rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeDecimal32Field(int32_t significand, int8_t scale,
    rscp_payloadBuffer_t *buffer);

void rscp_txBinaryIOChannelChange (uint16_t channel, uint8_t state);
void rscp_txContinuousIOChannelChange(uint16_t channel, void *value, int8_t scale, uint8_t unit, rscp_fieldType fieldType);

#endif /* _RSCP_NET_H */

