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

#include <stdio.h>

#include "vscp_io.h"
#include "core/util/fifo.h"

#ifdef VSCP_SUPPORT

/* ---------------------------------------------------------------------------
 * get value of hardware inputs
 */
uint32_t
vscp_get_input(void)
{
  return ((((uint32_t) PINE << 24) |    // io4.0 - io4.7
           ((uint32_t) PINF << 16) |    // io3.0 - io3.7
           ((uint32_t) PINC << 8) |     // io2.0 - io2.7
           (uint32_t) PINA) ^   // io1.0 - io1.7
          ~VSCP_IO_DIRECTION);
}


/* ---------------------------------------------------------------------------
 * set hardware outputs
 */
void
vscp_set_output(uint32_t value)
{
  uint32_t io = ~VSCP_IO_DIRECTION | value;
  PORTA = (uint8_t) (io);
  PORTC = (uint8_t) (io >> 8);
  PORTF = (uint8_t) (io >> 16);
  PORTE = (uint8_t) (io >> 24);
}


/* ---------------------------------------------------------------------------
 * set hardware io direction
 */
void
vscp_set_direction(uint32_t value)
{
  DDRA = (uint8_t) (value);
  DDRC = (uint8_t) (value >> 8);
  DDRF = (uint8_t) (value >> 16);
  DDRE = (uint8_t) (value >> 24);
  VSCP_DEBUG("IO-direction: 0x%08lX\n", value);
}


/* ---------------------------------------------------------------------------
 * set configure io
 */
void
vscp_io_init(void)
{
  FIFO_init(vscp_InputBuffer);
  vscp_set_direction(VSCP_IO_DIRECTION);
  vscp_set_output(0x00000000);
}


/* ---------------------------------------------------------------------------
 * debounce hardware inputs
 */
void
vscp_debounce(void)
{
  static uint32_t vscp_debounce0, vscp_debounce1, vscp_input;

  // test if any changes occurred
  uint32_t vscp_edge = vscp_get_input() ^ vscp_input;

  // increment the vertical counter
  // reset the counter if no change has occurred
  vscp_debounce1 = vscp_debounce0 ^ (vscp_debounce1 & vscp_edge);
  vscp_debounce0 = ~vscp_debounce0 & vscp_edge;

  // set edge detect if vertical counter has overflow (both bits are 0)
  vscp_edge &= ~(vscp_debounce0 | vscp_debounce1);

  // update debounced input
  vscp_input ^= vscp_edge;

  if (vscp_edge != 0)
  {
    if (((vscp_InputBuffer._write + 2) & (VSCP_INPUT_BUFFER_SIZE - 1)) !=
        vscp_InputBuffer._read)
    {
      FIFO_write(vscp_InputBuffer, vscp_edge, VSCP_INPUT_BUFFER_SIZE);
      FIFO_write(vscp_InputBuffer, vscp_input, VSCP_INPUT_BUFFER_SIZE);
      vscp_set_output((vscp_input << 8));
    }
    else
    {
      vscp_input ^= vscp_edge;  // delay debounce
    }
  }
}
#endif /* VSCP_SUPPORT */


/*
   -- Ethersex META --
   header(protocols/vscp/vscp_io.h)
   init(vscp_io_init)
   timer(1, vscp_debounce())
   block(Miscelleanous)
 */
