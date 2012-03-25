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
#define VSCP_ETHTYPE 9598
#define VSCP_UDPH_LEN 1      // size of upd header
#define VSCP_RAWH_LEN 14     // size of raw ethernet header
#define VSCP_CTGS_LEN 22     // size of class, type, guid and size
#define VSCP_CRC_LEN 2       // size of crc
#define VSCP_MAX_DATA (512 - 25)

#define VSCP_UDP_POS_HEAD  0
#define VSCP_UDP_POS_CLASS 1
#define VSCP_UDP_POS_TYPE  3
#define VSCP_UDP_POS_GUID  5
#define VSCP_UDP_POS_SIZE  21
#define VSCP_UDP_POS_DATA  23

#define VSCP_RAW_POS_HEAD  1
#define VSCP_RAW_POS_CLASS 11
#define VSCP_RAW_POS_TYPE  13
#define VSCP_RAW_POS_GUID  15
#define VSCP_RAW_POS_SIZE  31
#define VSCP_RAW_POS_DATA  33

#ifndef htonl
#define htonl(x) __builtin_bswap32(x)
#endif /* !htonl */
#ifndef ntohl
#define ntohl htonl
#endif /* !ntohl */


/* structs */
struct vscp_event
{  
  uint16_t class;     // VSCP class
  uint16_t type;      // VSCP type
  uint8_t  guid[16];  // node address MSB(0) -> LSB(15)
  uint16_t size;      // number of valid data bytes
  uint8_t  data[VSCP_MAX_DATA];  // data; max 487 (512- 25) bytes
};


struct vscp_udp_event
{
  uint8_t  head;        // bit 765:  prioriy 0-7 where 0 is highest
                        // bit 4:    hardcoded, true for a hardcoded device
                        // bit 3210: reserved
  struct vscp_event event;
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
  struct vscp_event event;
};


/* prototypes */
void vscp_net_init(void);
void vscp_net_udp(void);
void vscp_net_raw(void);

#endif /* _VSCP_NET_H */

