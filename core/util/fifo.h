/*
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

#ifndef FIFO_H_
#define FIFO_H_

#include <stdint.h>

/** example for a fifo of 64 bytes */
typedef struct
{
  uint8_t _read;
  uint8_t _write;
  uint8_t _buffer[64];
} FIFO64_t;

#define FIFO_init(fifo) { \
  fifo._read = 0; \
  fifo._write = 0; \
}

#define FIFO_available(fifo) ( \
  fifo._read != fifo._write \
)

#define FIFO_read(fifo, size) ( \
  fifo._buffer[fifo._read = (fifo._read + 1) & (size - 1)] \
)

#define FIFO_write(fifo, data, size) { \
  uint8_t tmphead = (fifo._write + 1) & (size - 1); \
  if(tmphead != fifo._read) { /* if buffer is not full */ \
    fifo._write = tmphead; \
    fifo._buffer[tmphead] = data; \
  } \
}

#define FIFO64_read(fifo)         FIFO_read(fifo, 64)
#define FIFO64_write(fifo, data)  FIFO_write(fifo, data, 64)

#endif /*FIFO_H_ */
