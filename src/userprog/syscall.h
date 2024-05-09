#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>

struct lock syscall_sync_lock; /* global lock file */

void syscall_init(void);

static struct lock files_sync_lock; // lock for syncronization between files

void sys_exit(int status);
void sys_halt();

#endif /* userprog/syscall.h */
