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

#if defined(RSCP_SUPPORT) && defined(ONEWIRE_SUPPORT) && defined(ONEWIRE_HOOK_SUPPORT)

#ifndef _RSCP_ONEWIRE
#define _RSCP_ONEWIRE

#define RSCP_ONEWIRE_SUPPORT

void rscp_parseOWC(void *ptr, uint16_t items, uint16_t firstChannelID);
void rscp_onewire_publish_state();

#endif
#endif
