/*
 * (c) 2012 by Frank Sautter <ethersix@sautter.com>
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

#ifndef _VSCP_IO_H
#define _VSCP_IO_H

#ifdef VSCP_SUPPORT

#include "vscp.h"
#include "hardware/input/buttons/buttons.h"

#define VSCP_IO_DIRECTION 0xFF00FF00

#define VSCP_INPUT_BUFFER_SIZE 16

typedef struct
{
  uint8_t _read;
  uint8_t _write;
  uint32_t _buffer[VSCP_INPUT_BUFFER_SIZE];
} vscp_InputBuffer_t;

vscp_InputBuffer_t vscp_InputBuffer;

void hook_btn_handler(btn_ButtonsType btn, uint8_t status);
void vscp_set_direction(uint32_t value);
void vscp_set_output(uint32_t value);
uint32_t vscp_get_input(void);
void vscp_io_init(void);
void vscp_debounce(void);

#endif /* VSCP_SUPPORT */
#endif /* _VSCP_IO_H */
