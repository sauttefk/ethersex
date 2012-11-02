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

void timer_init(timer *t, timer_callback cb, void *user) {
  *t = blank;
  TP(t)->callback = cb;
  TP(t)->user = user;
  TP(t)->next = user;
  t->state = TIMER_IDLE;
}

void _timer_schedule(timer *t) {
  timer *p;


  if(!timer_chain) {
    timer_chain = t;
    TP(t)->next = (timer*) 0;
    TIMER_DEBUG("scheduling: %hx@%lu.%hu as root\n", t, TP(t)->next_fire.seconds, TP(t)->next_fire.millis);
    return;
  }

  p = timer_chain;
  while(TP(p)->next != 0 && mtime_compare(&(TP(TP(p)->next)->next_fire), &(TP(t)->next_fire)) <= 0)
    p = TP(p)->next;

  timer *next = TP(p)->next;
  TP(p)->next = t;
  TP(t)->next = next;

  TIMER_DEBUG("scheduling: %hx@%lu.%hu after %hx@%lu.%hu\n",
      t, TP(t)->next_fire.seconds, TP(t)->next_fire.millis,
      p, TP(p)->next_fire.seconds, TP(p)->next_fire.millis);
}

void timer_schedule_at_mtime(timer *t, mtime *time) {

  TP(t)->next_fire = *time;
  TP(t)->interval = zeroTime;

  timer_cancel(t);
  _timer_schedule(t);

  t->state = TIMER_SCHEDULED;
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

  timer_cancel(t);
  _timer_schedule(t);

  t->state = TIMER_SCHEDULED;
}

void timer_schedule_repeating_msecs(timer *t, uint32_t initial, uint32_t interval) {
  timer_schedule_after_msecs(t, initial);
  mtime_from_milliseconds(&(TP(t)->interval), interval);

  t->state = TIMER_SCHEDULED_REPEATING;
}

void timer_schedule_repeating_mtime(timer *t, mtime *initial, mtime *interval) {
  timer_schedule_after_mtime(t, initial);
  TP(t)->interval = *interval;

  t->state = TIMER_SCHEDULED_REPEATING;
}

void timer_cancel(timer *t) {
  if(!timer_chain)
    return;

  if(t == timer_chain) {
    timer_chain = TP(timer_chain)->next;
    TP(t)->next = 0;
    t->state = TIMER_CANCELED;

    TIMER_DEBUG("canceled first timer, next is now %hx\n", timer_chain);
    return;
  }

  timer *p = timer_chain;
  while(TP(p)->next != 0 && TP(p)->next != t)
    p = TP(p)->next;

  if(TP(p)->next == t) {
    TP(p)->next = TP(t)->next;
    TP(t)->next = 0;
    t->state = TIMER_CANCELED;

    TIMER_DEBUG("canceled %hx between %hx and %hx \n", t, p, TP(p)->next);
  } else
    TIMER_DEBUG("timer %hx to be canceled not found\n", t);
}

#include "services/clock/clock.h"

void timer_periodic() {
  mtime now;

  // fire all the timers which have elapsed
  while(1) {
    if(timer_chain == 0) {
      TIMER_DEBUG("no pending timer\n");
      break;
    }

    mtime_get_current(&now);

    mtime *nextFireAt = &(TP(timer_chain)->next_fire);

    TIMER_DEBUG("now: %lu.%hu, first timer: %hx@%lu.%hu\n", now.seconds, now.millis, timer_chain, nextFireAt->seconds, nextFireAt->millis);

    if(nextFireAt->seconds < now.seconds || (nextFireAt->seconds == now.seconds && nextFireAt->millis <= now.millis)) {
      timer *toFire = timer_chain;

      // remove from chain
      timer_chain = TP(toFire)->next;

      // re-add if scheduled at interval
      uint8_t repeating = TP(toFire)->interval.seconds != 0 || TP(toFire)->interval.millis != 0;
      if(repeating) {
        TP(toFire)->next_fire = now;
        mtime_add(&(TP(toFire)->next_fire), &(TP(toFire)->interval));
        TIMER_DEBUG("re-scheduling at %lu.%hu\n", TP(toFire)->next_fire.seconds, TP(toFire)->next_fire.millis);
        _timer_schedule(toFire);
      }

      // fire
      (TP(toFire)->callback)(toFire, TP(toFire)->user);

      if(!repeating)
        toFire->state = TIMER_EXPIRED;

      TIMER_DEBUG("fired %hx with callback %hx, next to fire is now %hx@%lu.%hu\n",
          toFire, TP(toFire)->callback,
          timer_chain, timer_chain ? TP(timer_chain)->next_fire.seconds : 0, timer_chain ? TP(timer_chain)->next_fire.millis : 0);

    } else
      break;
  }
}

#ifdef TIMER_TESTS
#include "core/debug.h"

void timer_a(timer *t, void *user) {
  debug_printf("timer A: %s\n", (char*)user);
}
void timer_b(timer *t, void *user) {
  static uint8_t count = 0;

  debug_printf("timer B: %s %d\n", (char*)user, count);

  if(count++ > 20)
    timer_cancel(t);
}
void timer_c(timer *t, void *user) {
  static uint8_t count = 0;

  debug_printf("timer C: %s %d\n", (char*)user, count);

  if(count++ < 10)
    timer_schedule_after_msecs(t, 1000);
}

void timer_run_tests(void) {
  static timer timerA, timerB, timerC;

  debug_printf("running tests\n");

  timer_init(&timerA, &timer_a, "foo");
  timer_init(&timerB, &timer_b, "bar");
  timer_init(&timerC, &timer_c, "baz");

  timer_schedule_after_msecs(&timerA, 2000);

  timer_schedule_repeating_msecs(&timerB, 4000, 2000);

  timer_schedule_after_msecs(&timerC, 3000);
}
#endif

/*
  -- Ethersex META --
  header(`protocols/rscp/timer.h')
  ifdef(`TIMER_TESTS',`init(timer_run_tests)')
  timer(1, timer_periodic())
  block(Miscelleanous)
 */
