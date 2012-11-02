/*
 * mtime.h
 *
 * Time-keeping with millisecond-resolution.
 */

#ifndef MTIME_H_
#define MTIME_H_

#include <stdint.h>
#include "core/periodic.h"

// The relationship between mtime and the periodic timer
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

/*
 * Compare the two mtime instances.
 */
int8_t mtime_compare(const mtime *a, const mtime *b);

/*
 * Set an mtime delay from a given millisecond value
 */
void mtime_from_milliseconds(mtime *t, const uint32_t milliseconds);

void mtime_add(mtime *a, const mtime *b);

#endif /* MTIME_H_ */
