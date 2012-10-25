/*
 * timer.c
 *
 *  Created on: 24.10.2012
 *      Author: hennejg
 */

#include "timer.h"

static timer *timer_chain = 0;

#define T_PRIVATE(t) ((timer_private*)t->stuff)

uint8_t timer_init(timer *t, timer_callback *cb, void *user) {
  *t = 0;
  T_PRIVATE(t)->callback = cb;
  T_PRIVATE(t)->user = user;

  return 0;
}

uint8_t _timer_schedule(timer *t) {
  timer *p;

  if(!timer_chain) {
    timer_chain = t;
    return 0;
  }

  p = timer_chain;
}

uint8_t timer_schedule_after_msecs(timer *t, uint32_t milliseconds) {
  T_PRIVATE(t)->next_fire.seconds = milliseconds / 1000;
  T_PRIVATE(t)->next_fire.millis = milliseconds % 1000;
  T_PRIVATE(t)->interval = 0;

  _timer_schedule(t);
}

uint8_t timer_schedule_after_mtime(timer *t, mtime time);
uint8_t timer_schedule_repeating_msecs(timer *t, uint32_t initial, uint32_t interval);
uint8_t timer_schedule_repeating_mtime(timer *t, mtime initial, mtime interval);

uint8_t timer_cancel(timer *t);

