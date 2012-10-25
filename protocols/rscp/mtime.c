#include "mtime.h"
#include "clock.h"

void mtime_get_current(mtime *t) {
 t->seconds = clock_get_time();
 t->millis = clock_get_ticks() * MTIME_MILLIS_PER_TICK;
}
