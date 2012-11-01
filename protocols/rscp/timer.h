/*
 * timer.h
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "mtime.h"

typedef struct timer timer;

typedef void (*timer_callback)(timer *timer, void *user);

typedef enum {
  TIMER_IDLE = 0,
  TIMER_SCHEDULED,
  TIMER_SCHEDULED_REPEATING,
  TIMER_EXPIRED
} timer_state;

typedef struct timer_private {
  mtime next_fire;
  mtime interval;
  timer_callback callback;
  void *user;
  timer *next;
} timer_private;

/*
 * A definition of a timer allowing one-shot and repeated schedules.
 */
typedef struct timer {
  timer_state state;
  uint8_t stuff[sizeof(timer_private)];
} timer;

void timer_init(timer *t, timer_callback *cb, void *user);

void timer_schedule_at_mtime(timer *t, mtime *time);

void timer_schedule_after_msecs(timer *t, uint32_t milliseconds);
void timer_schedule_after_mtime(timer *t, mtime *delay);

void timer_schedule_repeating_msecs(timer *t, uint32_t delay, uint32_t interval);
void timer_schedule_repeating_mtime(timer *t, mtime *delay, mtime *interval);

void timer_cancel(timer *t);

void timer_periodic(void);

#endif /* TIMER_H_ */
