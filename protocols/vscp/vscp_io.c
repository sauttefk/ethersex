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

uint32_t
vscp_get_io(void)
{
  return(((uint32_t) PORTE << 24) + // io4.0 - io4.7
         ((uint32_t) PORTF << 16) + // io3.0 - io3.7
         ((uint32_t) PORTC << 8)  + // io2.0 - io2.7 
          (uint32_t) PORTA)        	// io1.0 - io1.7
}


void
vscp_set_io(uint32_t value)
{
  PORTA = value & 0xFF;
  PORTC = (value >> 8) & 0xFF;
  PORTF = (value >> 16) & 0xFF;
  PORTE = (value >> 24) & 0xFF;
}

void
vscp_debounce(void)
{
  static uint32_t vscp_debounce0, vscp_debounce1;
  uint32_t i = key_state ^ ~KEY_PIN;    // key changed ?
  vscp_debounce0 = ~( vscp_debounce0 & i );          // reset or count vscp_debounce0
  vscp_debounce1 = vscp_debounce0 ^ (vscp_debounce1 & i);       // reset or count vscp_debounce1
  i &= vscp_debounce0 & vscp_debounce1;              // count until roll over ?
  key_state ^= i;              // then toggle debounced state
  key_press |= key_state & i;  // 0->1: key press detect
}
