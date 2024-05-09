#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>

struct lock syscall_sync_lock; /* global lock file */

void syscall_init(void);

static struct lock files_sync_lock;           // lock for syncronization between files
int get_int(struct intr_frame *f);            // get int from the stack
char *get_char_ptr(char ***esp);              // get char ptr from the stack
void *get_void_ptr(void **esp);               // get void ptr from the stack
bool validate_void_ptr(struct intr_frame *f); // check if the pointer is valid

void sys_exit(int status);
void sys_halt();
void sys_seek(struct intr_frame *f);
void sys_tell(struct intr_frame *f);

void wrapper_sys_exit(struct intr_frame *f);
void wrapper_sys_exec(struct intr_frame *f);
void wrapper_sys_wait(struct intr_frame *f);
void wrapper_sys_create(struct intr_frame *f);
void wrapper_sys_remove(struct intr_frame *f);
void wrapper_sys_open(struct intr_frame *f);
void wrapper_sys_filesize(struct intr_frame *f);
void wrapper_sys_read(struct intr_frame *f);
void wrapper_sys_write(struct intr_frame *f);
void wrapper_sys_close(struct intr_frame *f);

#endif /* userprog/syscall.h */
