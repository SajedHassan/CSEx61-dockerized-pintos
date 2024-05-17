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
#include "filesys/filesys.h"
#include "filesys/file.h"

static void syscall_handler(struct intr_frame *);

int get_int(struct intr_frame *f);            // get int from the stack
bool validate_void_ptr(struct intr_frame *f); // check if the pointer is valid
bool validate_virtual_memory(void *val);

void sys_seek(struct intr_frame *f);
void sys_tell(struct intr_frame *f);

void wrapper_sys_exit(struct intr_frame *f);
void wrapper_sys_exec(struct intr_frame *f);
void wrapper_sys_wait(struct intr_frame *f);
/*=================================================*/

void wrapper_sys_create(struct intr_frame *f);
void wrapper_sys_remove(struct intr_frame *f);
void wrapper_sys_open(struct intr_frame *f);
void wrapper_sys_filesize(struct intr_frame *f);
void wrapper_sys_read(struct intr_frame *f);
void wrapper_sys_write(struct intr_frame *f);
void wrapper_sys_close(struct intr_frame *f);

int sys_create(const char *file, unsigned initial_size);

void close_file(int fd);
int read_file(int fd, void *buffer, unsigned size);
int filesize(int fd);

/*=================================================*/

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
    printf("CALLING CREATE\n");
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

/*======================================================================================================================================*/
/*======================================================FILES SYSTEM ===================================================================*/
/*======================================================================================================================================*/


/**
 * @brief Changes the next byte to be read or written in open file to position,
 * expressed in bytes from the beginning of the file. (Thus, a position of 0 is the file's start.)
 * A seek past the current end of a file is not an error. A later read obtains
 * 0 bytes, indicating end of file. A later write extends the file, filling any
 * unwritten gap with zeros. (However, in Pintos files have a fixed length until
 * project 4 is complete, so writes past end of file will return an error.)
 * These semantics are implemented in the file system and do not require any
 * special effort in system call implementation.
 * 
 * @param f 
 */
void sys_seek(struct intr_frame *f) {
    int fd = *((int *)f->esp + 1);
    unsigned int position = *((unsigned int *)f->esp + 2);
    struct file *file = file_open(fd);
    if (file == NULL)
      sys_exit(-1); 
    file_seek(file, position);
}
/**
 * @brief Returns the position of the next byte to be read or written in open ﬁle fd,
 * expressed in bytes from the beginning of the ﬁle.
 * 
 * @param f 
 */
void sys_tell(struct intr_frame *f) {
  int fd = *((int *)f->esp + 1);
  struct file *file = file_open(fd);
  if (file == NULL)
    sys_exit(-1);
  f->eax = file_tell(file);
}

/**
 * @brief 
 * System Call: bool create (const char *file, unsigned initial_size)
 * Creates a new file called file initially initial_size bytes in size. 
 * Returns true if successful, false otherwise. 
 * Creating a new file does not open it: opening the new file is a separate operation which would require a open system call. 
 * 
 * @param f 
 */
void wrapper_sys_create(struct intr_frame *f) {
  char *file_name = (char *)(*((int *)f->esp + 1));
  printf("%s\n", file_name);
  if (!validate_virtual_memory(file_name))
  {
    sys_exit(-1);

  }
  unsigned int initial_size = *((unsigned int *)f->esp + 2);

  int TEMP = 0;
  lock_acquire(&syscall_sync_lock);
  TEMP = filesys_create(file_name, initial_size);
  lock_release(&syscall_sync_lock);
  f->eax = TEMP;
}

/**
 * @brief 
 * Deletes the file called file. Returns true if successful, false otherwise. 
 * A file may be removed regardless of whether it is open or closed, and removing an open file does not close it. 
 * 
 * @param f 
 */
void wrapper_sys_remove(struct intr_frame *f) {
  char *file_name = (char *)(*((int *)f->esp + 1));
  if (!validate_virtual_memory(file_name))
  {
    sys_exit(-1);
  }

  int TEMP = -1;
  lock_acquire(&syscall_sync_lock);
  TEMP = filesys_remove(file_name);
  lock_release(&syscall_sync_lock);
  f->eax = TEMP;
}

// TODO: Implement this function again

/**
 * @brief Opens the file called file. Returns a nonnegative integer handle called a "file descriptor" (fd), or -1 if the file could not be opened.

 * File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is standard input, fd 1 (STDOUT_FILENO) is standard output. 
 * The open system call will never return either of these file descriptors, which are valid as system call arguments only as explicitly described below.
 * Each process has an independent set of file descriptors. File descriptors are not inherited by child processes.
 * When a single file is opened more than once, whether by a single process or different processes, each open returns a new file descriptor. Different file descriptors for a single file are closed independently in separate calls to close and they do not share a file position. 
 * 
 * @param f 
 */
void wrapper_sys_open(struct intr_frame *f) {
  char *file_name = (char *)(*((int *)f->esp + 1));
  if (!validate_virtual_memory(file_name))
  {
    sys_exit(-1);
  }

  int TEMP = 0;
  lock_acquire(&syscall_sync_lock);
  TEMP = filesys_open(file_name);
  lock_release(&syscall_sync_lock);
  f->eax = TEMP;
}

// TODO: Implement this function again


/**
 * @brief Returns the size, in bytes, of the file open as fd.
 * 
 * @param f 
 */
void wrapper_sys_filesize(struct intr_frame *f) {
  int fd = *((int *)f->esp + 1);
  f->eax = filesize(fd);
}
/**
 * @brief Reads size bytes from the file open as fd into buffer. 
 * Returns the number of bytes actually read (0 at end of file), or -1 if the file could not be read (due to a condition other than end of file). 
 * Fd 0 reads from the keyboard using input_getc(). 
 * 
 * @param f 
 */
void wrapper_sys_read(struct intr_frame *f) {
  int fd = *((int *)f->esp + 1);
  char *buffer = (char *)(*((int *)f->esp + 2));
  unsigned int size = *((unsigned int *)f->esp + 3);
  if (fd == 0) {
    // Read from the keyboard using input_getc()
    int i;
    for (i = 0; i < size; i++) {
      buffer[i] = input_getc();
    }
    f->eax = size;
  } else {
    f->eax = read_file(fd, buffer, size);
  }
}

/**
 * @brief Closes file descriptor fd. 
 * Exiting or terminating a process implicitly closes all its open file descriptors, as if by calling this function for each one. 
 * 
 * @param f 
 */
void wrapper_sys_close(struct intr_frame *f) {
  int fd = *((int *)f->esp + 1);
  close_file(fd);
}
/*============================================NOTES==============================================*/
// esp is the stack pointer, which points to the top of the current stack frame.
// Consider a function f() that takes three int arguments. 
// This diagram shows a sample stack frame as seen by the callee at the beginning of step 3 above, supposing that f() is invoked as f(1, 2, 3). 
// The initial stack address is arbitrary: 
//                              +----------------+
//                   0xbffffe7c |        3       |
//                   0xbffffe78 |        2       |
//                   0xbffffe74 |        1       |
// stack pointer --> 0xbffffe70 | return address |
//                              +----------------+

// f->esp is a memory address where an integer value is stored. 
//  acess the value stored by :  1- (int *)f->esp         ==> pointer to an integer
//                               2- *((int *)f->esp)      ==> value of the integer
//                               3- *((int *)f->esp + 1)  ==> value of the integer at the next address

/*=========================================HELPER FUNCTIONS====================================*/

int filesize(int fd)
{
  struct file *file = file_open(fd);
  if (file == NULL)
    return -1;
  return file_length(file);
}

int read_file(int fd, void *buffer, unsigned size)
{
  if (fd == 0)
  { // fd is 0, reads from the keyboard
    unsigned i;
    uint8_t *local_buffer = (uint8_t *)buffer;
    for (i = 0; i < size; i++)
    {
      local_buffer[i] = input_getc();
    }
    return size;
  }
  struct file *file = file_open(fd);
  if (file == NULL)
    return -1;
  return file_read(file, buffer, size);
}

void close_file(int fd)
{
  struct file *file = file_open(fd);
  if (file != NULL)
  {
    file_close(file);
  }
}
