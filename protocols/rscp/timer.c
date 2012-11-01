/*
 * timer.c
 *
 *  Created on: 24.10.2012
 *      Author: hennejg
 */

#include "timer.h"

static timer *timer_chain = 0;

#define TP(t) ((timer_private*)t->stuff)

const timer blank;
const mtime zeroTime;

void timer_init(timer *t, timer_callback *cb, void *user) {
  *t = blank;
  TP(t)->callback = cb;
  TP(t)->user = user;
  TP(t)->next = user;
}

void _timer_schedule(timer *t) {
  timer *p;

  if(!timer_chain) {
    timer_chain = t;
    TP(t)->next = (timer*) 0;
    return;
  }

  p = timer_chain;
  while(TP(p)->next != 0 && mtime_compare(&(TP(TP(p)->next)->next_fire), &(TP(t)->next_fire)) <= 0)
    p = TP(p)->next;

  timer *next = TP(p)->next;
  TP(p)->next = t;
  TP(t)->next = next;
}

void timer_schedule_at_mtime(timer *t, mtime *time) {
  TP(t)->next_fire = *time;
  TP(t)->interval = zeroTime;

  _timer_schedule(t);
}

void timer_schedule_after_msecs(timer *t, uint32_t milliseconds) {
  mtime delay;
  mtime_from_milliseconds(&delay, milliseconds);

  timer_schedule_after_mtime(t, &delay);
}

void timer_schedule_after_mtime(timer *t, mtime *delay) {
  mtime *nextFire = &(TP(t)->next_fire);
  mtime_get_current(nextFire);
  mtime_add(nextFire, delay);

  TP(t)->interval = zeroTime;
  _timer_schedule(t);
}

void timer_schedule_repeating_msecs(timer *t, uint32_t initial, uint32_t interval) {
  timer_schedule_after_msecs(t, initial);
  mtime_from_milliseconds(&(TP(t)->interval), interval);
}

void timer_schedule_repeating_mtime(timer *t, mtime *initial, mtime *interval) {
  timer_schedule_after_mtime(t, initial);
  TP(t)->interval = *interval;
}

void timer_cancel(timer *t) {
  if(!timer_chain)
    return;

  if(t == timer_chain) {
    timer_chain = TP(timer_chain)->next;
    TP(t)->next = 0;
    return;
  }

  timer *p = timer_chain;
  while(TP(p)->next != 0 && TP(p)->next != t)
    p = TP(p)->next;

  if(TP(p)->next == t) {
    TP(p)->next = TP(t)->next;
    TP(t)->next = 0;
  }
}

void timer_periodic() {
  // fire all the timers which have elapsed
  while(1) {
    if(timer_chain == 0)
      break;

    mtime now;
    mtime_get_current(&now);

    mtime *nextFire = &(TP(timer_chain)->next_fire);
    if(nextFire->seconds < now.seconds || (nextFire->seconds == now.seconds && nextFire->millis <= now.millis)) {
      timer *toFire = timer_chain;

      // remove from chain
      timer_chain = TP(toFire)->next;

      // fire
      (TP(toFire)->callback)(toFire, TP(toFire)->user);

      // re-add if scheduled at interval
      if(TP(toFire)->interval.seconds != 0 || TP(toFire)->interval.millis != 0) {
        TP(toFire)->next_fire = now;
        mtime_add(&(TP(toFire)->next_fire), &(TP(toFire)->interval));
        _timer_schedule(toFire);
      }
    } else
      break;
  }
}

/*
  -- Ethersex META --
  header(`protocols/rscp/timer.h')
  timer(1, timer_periodic())
  block(Miscelleanous)
 */
