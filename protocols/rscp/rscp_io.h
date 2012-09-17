/*
 * (c) 2012 by Frank Sautter <ethersix@sautter.com>
 * (c) 2012 by Daniel Walter <fordprfkt@googlemail.com>
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

#include "rscp.h"

#if !defined(CONF_NUM_BUTTONS) || !defined(BTN_CONFIG)
#error Error in pinning configuration for buttons module. Check your pinning \
configuration.
#endif

#define BUTTON_RELEASE    0         // button is not pressed
#define BUTTON_PRESS      1         // short press
#define BUTTON_LONGPRESS  2         // long press
#define BUTTON_REPEAT     3         // repeat function enabled, repeatedly triggered until button released
#define CONF_BTN_DEBOUNCE_TIME  3
#define CONF_BTN_LONGPRESS_TIME 50
#define CONF_BTN_REPEAT_TIME    100
#define CONF_BTN_REPEAT_RATE    200

/* These macros allow to use the the same configuration macro to initialize
 * the btn_ButtonsType enum, the button_configType struct and set the pullups.
 */
#define E(_v) _v,
#define C(_v) {.portIn = &PIN_CHAR(_v##_PORT), .pin = _v##_PIN},
#define PULLUP(_v) PIN_SET(_v);

typedef volatile uint8_t * const portPtrType;

/* Enum used in the cyclic function to loop over all buttons */
typedef enum
{
  BTN_CONFIG(E)
}btn_ButtonsType;

/* Static configuration data for each button */
typedef struct
{
  portPtrType portIn;
  const uint8_t pin;
} button_configType;

/* Status information for each button */
typedef struct
{
  uint8_t status        :2; // one of the values RELEASE, PRESS, LONGPRESS...
  uint8_t curStatus     :1; // current pin value
  uint8_t polarity      :1; // active polarity
  uint8_t reportPress   :1; // report button press
  uint8_t reportRelease :1; // report button release
  uint8_t unused        :2; // unused bits
  uint16_t debounce;        // debounce timer
  uint16_t repeat;          // repeat timer
} btn_statusType;

void rscp_io_init (void);
void buttons_periodic (void);
uint8_t get_button_state (uint16_t portID);
void rscp_button_handler (btn_ButtonsType button, uint8_t state,
                          uint16_t repeat);



#ifdef DEBUG_BUTTONS_INPUT
#include "core/debug.h"
#define BUTTONDEBUG(a...)  debug_printf("button: " a)
#else
#define BUTTONDEBUG(a...)
#endif

#endif /* RSCP_SUPPORT */
#endif /* _RSCP_IO_H */
