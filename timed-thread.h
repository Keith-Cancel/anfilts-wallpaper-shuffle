#ifndef TIMER_THREAD_H
#define TIMER_THREAD_H

#include <stdbool.h>
#include "thread-flex-bytes.h"

struct timer_thread_s;
typedef struct timer_thread_s TimerThread;

TimerThread* timer_thread_create(unsigned period_sec);
void         timer_thread_start(TimerThread* data);
void         timer_thread_close(TimerThread* data);

void         timer_thread_set_enable(TimerThread* data, bool enabled);
bool         timer_thread_is_enabled(TimerThread* data);
void         timer_thread_run_now(TimerThread* data);

unsigned     timer_thread_get_period(TimerThread* data);
bool         timer_thread_set_period(TimerThread* data, unsigned period_sec);

ThreadFlexBytes* timer_thread_get_search_dir(TimerThread* data);

#endif
