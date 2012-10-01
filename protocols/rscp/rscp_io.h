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

#ifndef _RSCP_IO_H
#define _RSCP_IO_H

#ifdef RSCP_SUPPORT

#ifdef DEBUG_RSCP_IO
#include "core/debug.h"
#define RSCP_DEBUG_IO(str...) debug_printf ("RSCP-IO: " str)
#else
#define RSCP_DEBUG_IO(...)    ((void) 0)
#endif


#if !defined(RSCP_IOS) || !defined(RSCP_IO_CONFIG)
#error Error in pinning configuration for buttons module. Check your pinning \
configuration.
#endif

enum
{
  BUTTON_RELEASE = 0,         // button is not pressed
  BUTTON_PRESS   = 1          // short press
};

/* These macros allow to use the the same configuration macro to initialize
 * the rscp_io_t enum, the ioConfig_t struct and set the pullups.
 */
#define E(_v) _v,
#define C(_v) {.portIn = &PIN_CHAR(_v##_PORT), \
               .portOut = &PORT_CHAR(_v##_PORT), \
               .ddr = &DDR_CHAR(_v##_PORT), \
               .pin = _v##_PIN},

typedef volatile uint8_t * const portPtrType;

/* Enum used in the cyclic function to loop over all buttons */
typedef enum
{
  RSCP_IO_CONFIG(E)
} rscp_io_t;

/* Static configuration data for each input */
typedef struct
{
  portPtrType portIn;
  portPtrType portOut;
  portPtrType ddr;
  const uint8_t pin;
} ioConfig_t;


void rscp_io_init (void);
void rscp_IOChannels_periodic (void);
uint8_t rscp_setPortDDR(uint16_t portID, uint8_t value);
uint8_t rscp_setPortPORT(uint16_t portID, uint8_t value);
uint8_t rscp_togglePortPORT(uint16_t portID);
uint8_t rscp_getPortPIN(uint16_t portID);


#ifdef DEBUG_BUTTONS_INPUT
#include "core/debug.h"
#define BUTTONDEBUG(a...)  debug_printf("button: " a)
#else
#define BUTTONDEBUG(a...)
#endif

#endif /* RSCP_SUPPORT */
#endif /* _RSCP_IO_H */
