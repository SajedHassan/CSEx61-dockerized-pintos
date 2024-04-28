#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>
#include "lib/kernel/list.h"/*----for usage of list abstract data types----*/
#include "threads/synch.h"/*----for usage of semaphores for locking----*/
#include "threads/thread.h"/*----for usage of threads functions----*/

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100

/*----struct for storing data about each element inside the sleeping list----*/
struct sleepy_thread_elem
{
    struct list_elem list_elem_val ; //the element value inside the linked list
    int64_t tick_to_walke_up ; //expected time for walke up in ticks
    struct semaphore* thread_sema; //semaphore for synchronization
    struct thread* current_thread; //data about thread 

};
/*----*******************************************************************----*/

void timer_init (void);
void timer_calibrate (void);

int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

/* Sleep and yield the CPU to other threads. */
void timer_sleep (int64_t ticks);
void timer_msleep (int64_t milliseconds);
void timer_usleep (int64_t microseconds);
void timer_nsleep (int64_t nanoseconds);

/* Busy waits. */
void timer_mdelay (int64_t milliseconds);
void timer_udelay (int64_t microseconds);
void timer_ndelay (int64_t nanoseconds);

void timer_print_stats (void);

#endif /* devices/timer.h */
