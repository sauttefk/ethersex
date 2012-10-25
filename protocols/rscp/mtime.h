/*
 * timestamp.h
 */

#ifndef MTIME_H_
#define MTIME_H_

#include <stdint.h>
#include "periodic.h"

#define MTIME_MILLIS_PER_TICK (1000 / HZ)

/*
 * A type definition for an absolute point in time or an elapsed
 * time with millisecond resolution.
 */
typedef struct {
  // The time in seconds. If this is an absolute time, it corresponds to a unix timestamp
  uint32_t seconds;
  // The remainder in milliseconds. Always smaller than one thousand.
  uint16_t millis;
} mtime;

/*
 * Store the current mtime in the mtime pointed to by t.
 */
void mtime_get_current(mtime *t);

#endif /* MTIME_H_ */
