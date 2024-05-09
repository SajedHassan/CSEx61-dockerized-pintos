#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "lib/kernel/list.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "userprog/process.h"

static void syscall_handler(struct intr_frame *);

int get_int(struct intr_frame *f);            // get int from the stack
bool validate_void_ptr(struct intr_frame *f); // check if the pointer is valid

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

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&syscall_sync_lock);
}

static void
syscall_handler(struct intr_frame *f)
{
  if (!validate_void_ptr(f))
  {
    sys_exit(-1);
  }

  switch (get_int(f))
  {
  case SYS_HALT:
  {
    sys_halt();
    break;
  }
  case SYS_EXIT:
  {
    wrapper_sys_exit(f);
    break;
  }
  case SYS_EXEC:
  {
    wrapper_sys_exec(f);
    break;
  }
  case SYS_WAIT:
  {
    wrapper_sys_wait(f);
    break;
  }
  case SYS_CREATE:
  {
    wrapper_sys_create(f);
    break;
  }
  case SYS_REMOVE:
  {
    wrapper_sys_remove(f);
    break;
  }
  case SYS_OPEN:
  {
    wrapper_sys_open(f);
    break;
  }
  case SYS_FILESIZE:
  {
    wrapper_sys_filesize(f);
    break;
  }
  case SYS_READ:
  {
    wrapper_sys_read(f);
    break;
  }
  case SYS_WRITE:
  {
    wrapper_sys_write(f);
    break;
  }
  case SYS_SEEK:
  {
    sys_seek(f);
    break;
  }
  case SYS_TELL:
  {
    sys_tell(f);
    break;
  }
  case SYS_CLOSE:
  {
    wrapper_sys_close(f);
    break;
  }
  default:
  {
    //
  }
  }
}

int get_int(struct intr_frame *f)
{
  return *(int *)(f->esp);
}

bool validate_virtual_memory(void *val)
{
  return val != NULL && is_user_vaddr(val) && pagedir_get_page(thread_current()->pagedir, val) != NULL;
}

bool validate_void_ptr(struct intr_frame *f)
{
  return validate_virtual_memory((int *)f->esp) || ((*(int *)f->esp) < 0) || (*(int *)f->esp) > 12;
}

void sys_exit(int status)
{
  char *name = thread_current()->name;
  char *save_ptr;
  char *executable = strtok_r(name, " ", &save_ptr);
  thread_current()->thread_exiting_status = status;
  printf("%s: exit(%d)\n", executable, status);
  thread_exit();
}

void sys_halt()
{
  printf("shutdown power...\n");
  shutdown_power_off();
}

void wrapper_sys_exit(struct intr_frame *f)
{
  int status = get_int(f);
  if (!is_user_vaddr(status))
  {
    f->eax = -1;
    sys_exit(-1);
  }
  f->eax = status;
  sys_exit(status);
}

tid_t sys_wait(tid_t tid)
{
  return process_wait(tid);
}

int sys_write(int fd, const void *buffer, unsigned size)
{
  if (fd == 1)
  { // fd is 1, writes to the console
    lock_acquire(&files_sync_lock);
    putbuf(buffer, size);
    lock_release(&files_sync_lock);
    return size;
  }

  // write to the file.
}

void sys_seek(struct intr_frame *f) {}

void sys_tell(struct intr_frame *f) {}

void wrapper_sys_wait(struct intr_frame *f)
{
  if (!validate_virtual_memory((int *)f->esp + 1))
    sys_exit(-1);
  f->eax = sys_wait(*((int *)f->esp + 1));
}

void wrapper_sys_exec(struct intr_frame *f)
{
  char *file_name = (char *)(*((int *)f->esp + 1));
  f->eax = process_execute(file_name);
}

void wrapper_sys_write(struct intr_frame *f)
{
  int fd = *((int *)f->esp + 1);
  char *buffer = (char *)(*((int *)f->esp + 2));
  if (fd == 0 || !validate_virtual_memory(buffer))
  { // fail, if fd is 0 (stdin), or its virtual memory
    sys_exit(-1);
  }
  unsigned size = (unsigned)(*((int *)f->esp + 3));
  f->eax = sys_write(fd, buffer, size);
}

void wrapper_sys_create(struct intr_frame *f) {}

void wrapper_sys_remove(struct intr_frame *f) {}

void wrapper_sys_open(struct intr_frame *f) {}

void wrapper_sys_filesize(struct intr_frame *f) {}

void wrapper_sys_read(struct intr_frame *f) {}

void wrapper_sys_close(struct intr_frame *f) {}