/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

void thread_priority_donation(struct thread *thread, struct lock *lock);

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void sema_init(struct semaphore *sema, unsigned value)
{
  ASSERT(sema != NULL);

  sema->value = value;
  list_init(&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
// ***********************************************************************************************************
void sema_down(struct semaphore *sema)
{
  // semaWait
  // The lock is already in use, So just wait until the thread release it
  enum intr_level old_level;

  ASSERT(sema != NULL);
  ASSERT(!intr_context());

  old_level = intr_disable();
  while (sema->value == 0)
  {
    list_push_back(&sema->waiters, &thread_current()->elem);
    thread_block();
  }
  sema->value--;
  intr_set_level(old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool sema_try_down(struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT(sema != NULL);

  old_level = intr_disable();
  if (sema->value > 0)
  {
    sema->value--;
    success = true;
  }
  else
    success = false;
  intr_set_level(old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
  and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void sema_up(struct semaphore *sema)
{
  // semaSignal
  enum intr_level old_level;
  ASSERT(sema != NULL);
  old_level = intr_disable();

  // First Modification **********************************************************************************
  // if there are waiters, get the highest priority thread from the list
  // and unblock it..

  // if (!list_empty(&sema->waiters))
  // {
  //   struct list_elem *highest_priority = get_max_priority_from_list(&sema->waiters);
  //   struct thread *chosen = list_entry(highest_priority, struct thread, elem);
  //   list_remove(highest_priority);
  //   thread_unblock(chosen);
  // }
   if (!list_empty (&sema->waiters)) 
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem));

  sema->value++;
  intr_set_level(old_level);
}

static void sema_test_helper(void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void sema_self_test(void)
{
  struct semaphore sema[2];
  int i;

  printf("Testing semaphores...");
  sema_init(&sema[0], 0);
  sema_init(&sema[1], 0);
  thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
  {
    sema_up(&sema[0]);
    sema_down(&sema[1]);
  }
  printf("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper(void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
  {
    sema_down(&sema[0]);
    sema_up(&sema[1]);
  }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void lock_init(struct lock *lock)
{
  ASSERT(lock != NULL);

  lock->holder = NULL;
  sema_init(&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void lock_acquire(struct lock *lock)
{
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(!lock_held_by_current_thread(lock));

  // Second Modification ***************************************************************************
 // struct thread *cur_thread = thread_current();
 // cur_thread->lock_wait_for = lock;
  // as if the thread has priority greater than the lock
  // the thread gives the lock the priority
  // like doctor and student :)
//  thread_priority_donation(cur_thread, lock);
  // scheduling needed after thread_priority_donation (changing priorities)
  
 //  thread_yield();
  sema_down(&lock->semaphore); // wait for the lock to be released
 // cur_thread->lock_wait_for = NULL;
  // as locks differ from semaphore that the lock has an owner
  // a holder
  lock->holder = thread_current (); //cur_thread;
  //list_push_front(&cur_thread->list_of_locks, &lock->elem);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool lock_try_acquire(struct lock *lock)
{
  bool success;

  ASSERT(lock != NULL);
  ASSERT(!lock_held_by_current_thread(lock));

  success = sema_try_down(&lock->semaphore);
  if (success)
    lock->holder = thread_current();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void lock_release(struct lock *lock)
{
  ASSERT(lock != NULL);
  ASSERT(lock_held_by_current_thread(lock));

  // Third Modification ***********************************************************************************
  // before releasing the lock, return holder priority to its main priority
  // and for the variable donated, okay i'm not donation any more

  // struct thread *holder = lock->holder;
  // holder->priority = holder->org_priority;
  // holder->donated = 0;
  // list_remove(&lock->elem);

  // when returning the holder priority to its main priority,
  // assume the holder has a lock with priority 100 and before returning to the main
  // priority the donated priority was 120, and its main priority 50
  // so we need to iterate over the all locks to guarantee that
  // the holder has the priority the same as its highest lock
  // holder list of locks
  // struct list *locks = &holder->list_of_locks;
  // struct list_elem *cur_lock = list_begin(locks);
  // while (cur_lock != list_end(locks))
  // {
  //   // get the value of the lock, then update it to the next pointer
  //   struct lock *cur_lock_entry = list_entry(cur_lock, struct lock, elem);
  //   cur_lock = list_next(cur_lock);

  //   struct semaphore *sema = &cur_lock_entry->semaphore;
  //   struct list *list_of_waiters = &sema->waiters;
  //   if (list_empty(list_of_waiters))
  //     continue;
  //   struct thread *highest_priority_thread = list_entry(get_max_priority_from_list(list_of_waiters), struct thread, elem);
  //   if (highest_priority_thread->priority > holder->priority)
  //   {
  //     holder->priority = highest_priority_thread->priority;
  //     holder->donated = 1;
  //   }
  // }

  lock->holder = NULL;       // No one is holding the lock now
  sema_up(&lock->semaphore); // another thread could take the lock
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool lock_held_by_current_thread(const struct lock *lock)
{
  ASSERT(lock != NULL);
  return lock->holder == thread_current();
}

/* One semaphore in a list. */
struct semaphore_elem
{
  struct list_elem elem;      /* List element. */
  struct semaphore semaphore; /* This semaphore. */
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void cond_init(struct condition *cond)
{
  ASSERT(cond != NULL);

  list_init(&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void cond_wait(struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  sema_init(&waiter.semaphore, 0);
  list_push_back(&cond->waiters, &waiter.elem);
  /* // Oooooooooooooooooooooooooooooooooooooooooops
  // Modify to insert thread at waiters list in order of priority
  list_insert_ordered(&cond->waiters, &waiter.elem, cmp_priority, NULL);
  */
  lock_release(lock);
  sema_down(&waiter.semaphore);
  lock_acquire(lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_signal(struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  // condition struct consists of list of waiters (semaphore_elem) only
  // semaphore struct consists values, and list of waiters only
  // semaphore_elem struct consists of semaphore, and list_elem

  // Fourth Modification **************************************************************************************
  // we need to wake the highest priority thread from the cond waiters
  // int max_priority = 0;
  // struct semaphore_elem *max_sema;
  // struct list_elem *cur_waiter = list_begin(&cond->waiters);
  // while (cur_waiter != list_end(&cond->waiters))
  // {
  //   struct semaphore_elem *cur_waiter_entry = list_entry(cur_waiter, struct semaphore_elem, elem);
  //   struct list_elem *max = get_max_priority_from_list(&((cur_waiter_entry->semaphore).waiters));
  //   int priority = list_entry(max, struct thread, elem)->priority;
  //   if (priority > max_priority)
  //   {
  //     max_priority = priority;
  //     max_sema = cur_waiter_entry;
  //   }

  //   cur_waiter = list_next(cur_waiter);
  // }

  // if (max_sema != NULL)
  // {
  //   sema_up(&max_sema->semaphore);
  //   list_remove(&max_sema->elem);
  // }
  if (!list_empty (&cond->waiters)) 
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                          struct semaphore_elem, elem)->semaphore);
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_broadcast(struct condition *cond, struct lock *lock)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);

  while (!list_empty(&cond->waiters))
    cond_signal(cond, lock);
}

// void thread_priority_donation(struct thread *thread, struct lock *lock)
// {
//   while (lock != NULL)
//   {
//     struct thread *holder = lock->holder;
//     if (holder != NULL && holder->priority < thread->priority)
//     {
//       // intrrupt disable to change holder priority
//       enum intr_level old_level;
//       old_level = intr_disable();
//       holder->priority = thread->priority;
//       intr_set_level(old_level);
//       // yes, i donated priority
//       holder->donated = 1;
//       // update variables
//       thread = holder;
//       lock = holder->lock_wait_for;
//       holder = lock->holder;
//     } else break;
//   }
// }
