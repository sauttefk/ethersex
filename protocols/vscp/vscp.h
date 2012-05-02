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
#include "vscp_net.h"
#include "vscp_class.h"
#include "vscp_type.h"
#include "vscp_firmware_level2.h"

#ifndef _VSCP_H
#define _VSCP_H

#ifdef VSCP_SUPPORT

#ifdef DEBUG_VSCP
#include "core/debug.h"
#define VSCP_DEBUG(str...) debug_printf ("vscp: " str)
#else
#define VSCP_DEBUG(...)    ((void) 0)
#endif

extern uint8_t vscp_mode;

void vscp_setup(void);
void vscp_main(void);
void vscp_get(uint8_t mode, uint16_t class, uint16_t type, uint16_t size,
              uint8_t *guid, uint8_t *payload);

void vscp_periodic(void);
void vscp_sendHeartBeat(void);
void sendPeriodicOutputEvents(void);
void sendPeriodicInputEvents(void);

void vscp_readRegister(uint8_t mode, uint8_t *payload);
void vscp_writeRegister(uint8_t mode, uint8_t *payload);
void vscp_getMatrixinfo(uint8_t mode, uint8_t *payload);

#define FIRMWARE_MAJOR_VERSION        0x00
#define FIRMWARE_MINOR_VERSION        0x00
#define FIRMWARE_SUB_MINOR_VERSION    0x01

#endif /* VSCP_SUPPORT */
#endif /* _VSCP_H */
