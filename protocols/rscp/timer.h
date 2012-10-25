/*
 * timer.h
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "mtime.h"

typedef void (*timer_callback)(void *user);

typedef enum {
  TIMER_IDLE = 0,
  TIMER_SCHEDULED,
  TIMER_SCHEDULED_REPEATING,
  TIMER_EXPIRED
} timer_state;

typedef struct {
  mtime next_fire;
  mtime interval;
  timer_callback *callback;
  void *user;
  timer *next;
} timer_private;

/*
 * A definition of a timer allowing one-shot and repeated schedules.
 */
typedef struct {
  timer_state state;
  uint8_t stuff[sizeof(timer_private)];
} timer;

uint8_t timer_init(timer *t, timer_callback *cb, void *user);
uint8_t timer_schedule_after_msecs(timer *t, uint32_t milliseconds);
uint8_t timer_schedule_after_mtime(timer *t, mtime time);
uint8_t timer_schedule_repeating_msecs(timer *t, uint32_t initial, uint32_t interval);
uint8_t timer_schedule_repeating_mtime(timer *t, mtime initial, mtime interval);

uint8_t timer_cancel(timer *t);

#endif /* TIMER_H_ */
