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
#include "rscp_net.h"

#ifndef _RSCP_H
#define _RSCP_H

#ifdef RSCP_SUPPORT

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

#define RSCP_SIZE_DEVURL              32
#define FIRMWARE_MAJOR_VERSION        0x00
#define FIRMWARE_MINOR_VERSION        0x00
#define FIRMWARE_SUB_MINOR_VERSION    0x01

#define RSCP_CHANNEL_EVENT            0x1001

#endif /* RSCP_SUPPORT */
#endif /* _RSCP_H */
