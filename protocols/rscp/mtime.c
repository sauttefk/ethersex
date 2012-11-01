#include "mtime.h"
#include "services/clock/clock.h"

void mtime_get_current(mtime *t) {
 t->seconds = clock_get_time();
 t->millis = clock_get_ticks() * MTIME_MILLIS_PER_TICK;
}

int8_t mtime_compare(const mtime *a, const mtime *b) {
  if(a->seconds < b->seconds)
    return -1;
  if(a->seconds > b->seconds)
    return 1;
  if(a->millis < b->millis)
    return -1;
  if(a->millis > b->millis)
    return 1;
  return 0;
}

void mtime_from_milliseconds(mtime *t, const uint32_t milliseconds) {
  t->seconds = milliseconds / 1000;
  t->millis = milliseconds - t->seconds * 1000;
}

void mtime_add(mtime *a, const mtime *b) {
  a->millis += b->millis; // no overrun, if mtimes are well-formed
  a->seconds += b->seconds;

  // normalize possible millis > 1000
  uint32_t seconds = a->millis / 1000;
  if(seconds > 0) {
    a->seconds += seconds;
    a->millis -= seconds * 1000;
  }
}
