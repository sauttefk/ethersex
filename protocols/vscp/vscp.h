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

#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "config.h"
#include "protocols/vscp/vscp_net.h"
#ifndef _VSCP_H
#define _VSCP_H

#ifdef VSCP_SUPPORT


#ifdef DEBUG_VSCP
#include "core/debug.h"
#define VSCP_DEBUG(str...) debug_printf ("vscp: " str)
#else
#define VSCP_DEBUG(...)    ((void) 0)
#endif


void vscp_init(void);
void vscp_main(void);
void vscp_get(struct vscp_event *event, int8_t frameType);
void vscp_createHead(struct vscp_event *event, int8_t frameType);
void vscp_readRegister(struct vscp_event *event, int8_t frameType);
void vscp_writeRegister(struct vscp_event *event, int8_t frameType);
void vscp_getMatrixinfo(struct vscp_event *event, int8_t frameType);


#endif /* _VSCP_H */
#endif /* VSCP_SUPPORT */
