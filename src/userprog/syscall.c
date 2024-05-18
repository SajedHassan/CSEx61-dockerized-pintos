#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h" // halt
#include "threads/init.h"     // halt
#include "threads/vaddr.h"    // exit
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "lib/kernel/list.h"

static void syscall_handler(struct intr_frame *);

// declaration for added wrapper methods __ system calls
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

// check validation
bool valid_in_virtual_memory(void *val);
bool valid_esp(struct intr_frame *f);
struct open_file *get_file(int fd);

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&required_lock);
}
////////////////////////////
static void
syscall_handler(struct intr_frame *f UNUSED)
{
  if (!valid_esp(f))
  {
    sys_exit(-1);
  }
  // switch to system calls
  switch (*(int *)f->esp)
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

// added methods _ for system calls
void sys_halt()
{
  printf("(halt) begin\n");
  shutdown_power_off();
}
/////////////////
void sys_exit(int status)
{
  char *name = thread_current()->name;
  char *save_ptr;
  char *executable = strtok_r(name, " ", &save_ptr);
  thread_current()->exit_status = status;
  printf("%s: exit(%d)\n", executable, status);
  thread_exit();
}

tid_t sys_wait(tid_t tid)
{
  return process_wait(tid);
}

// added methods _ for wrapper system calls
/* it's job just passing the status to exit */
void wrapper_sys_exit(struct intr_frame *f)
{
  int status = *((int *)f->esp + 1);
  if (!is_user_vaddr(status))
  {
    // not user virtual address, then exit with failure
    f->eax = -1;
    sys_exit(-1);
  }
  f->eax = status;
  sys_exit(status);
}

/* it's job executing the process */
void wrapper_sys_exec(struct intr_frame *f)
{
  char *file_name = (char *)(*((int *)f->esp + 1));
  f->eax = process_execute(file_name);
}

/* it's job is checking validation in virtual memory if valid pass it to sys_wait() */
void wrapper_sys_wait(struct intr_frame *f)
{
  if (!valid_in_virtual_memory((int *)f->esp + 1))
    sys_exit(-1);
  tid_t tid = *((int *)f->esp + 1);
  f->eax = sys_wait(tid);
}

////////////////////////////////

/**
 * @brief This file contains the implementation of system call functions for user programs.
 *
 * The functions in this file handle the validation of virtual memory for system calls.
 * Virtual memory validation ensures that the user program is accessing valid memory locations
 * and prevents unauthorized access to kernel memory.
 *
 */
bool valid_in_virtual_memory(void *val)
{
  //is_user_vaddr  Returns true if VADDR is a user virtual address.
  //pagedir_get_page check validation of stack pointer
  return val != NULL && is_user_vaddr(val) && pagedir_get_page(thread_current()->pagedir, val) != NULL;
}

// check validation of stack pointer
bool valid_esp(struct intr_frame *f)
{
  int syscall_number = *(int *)f->esp;
  return valid_in_virtual_memory((int *)f->esp) || syscall_number < 0 || syscall_number > 12;
}


/*======================================================================================================================================*/
/*======================================================FILES SYSTEM ===================================================================*/
/*======================================================================================================================================*/

/**
 * @brief Changes the next byte to be read or written in open file to position,expressed in bytes from the beginning of the file. (Thus, a position of 0 is the file's start.)
 * A seek past the current end of a file is not an error. A later read obtains 0 bytes, indicating end of file. A later write extends the file, filling any
 * unwritten gap with zeros. (However, in Pintos files have a fixed length until project 4 is complete, so writes past end of file will return an error.)
 * These semantics are implemented in the file system and do not require any
 * special effort in system call implementation.
 * 
 * @param f 
 */
void sys_seek(struct intr_frame *f)
{
  int fd = (int)(*((int *)f->esp + 1));
  unsigned pos = (unsigned)(*((int *)f->esp + 2));
  struct open_file *opened_file = get_file(fd);
  if (opened_file == NULL)
  { // fail
    f->eax = -1;
  }
  else
  {
    lock_acquire(&required_lock);
    file_seek(opened_file->file, pos);
    f->eax = pos;
    lock_release(&required_lock);
  }
}
/**
 * @brief Returns the position of the next byte to be read or written in open ﬁle fd,
 * expressed in bytes from the beginning of the ﬁle.
 * 
 * @param f 
 */
void sys_tell(struct intr_frame *f)
{
  int fd = (int)(*((int *)f->esp + 1));
  struct open_file *opened_file = get_file(fd);
  if (opened_file == NULL)
  {
    f->eax = -1;
  }
  else
  {
    lock_acquire(&required_lock);
    f->eax = file_tell(opened_file->file);
    lock_release(&required_lock);
  }
}
/*========================================================================================================================*/
/**
 * @brief 
 * System Call: bool create (const char *file, unsigned initial_size)
 * Creates a new file called file initially initial_size bytes in size. 
 * Returns true if successful, false otherwise. 
 * Creating a new file does not open it: opening the new file is a separate operation which would require a open system call. 
 * 
 * @param f 
 */
void wrapper_sys_create(struct intr_frame *f)
{
  char *file_name = (char *)(*((int *)f->esp + 1));
  if (!valid_in_virtual_memory(file_name))
  {
    sys_exit(-1);
  }
  unsigned initial_size = (unsigned)(*((int *)f->esp + 2));
  int temp = 0;
  lock_acquire(&required_lock);
  temp = filesys_create(file_name, initial_size);
  lock_release(&required_lock);
  f->eax = temp;
}

/**
 * @brief 
 * Deletes the file called file. Returns true if successful, false otherwise. 
 * A file may be removed regardless of whether it is open or closed, and removing an open file does not close it. 
 * 
 * @param f 
 */
void wrapper_sys_remove(struct intr_frame *f)
{
  char *file = (char *)(*((int *)f->esp + 1));
  if (!valid_in_virtual_memory(file))
  {
    sys_exit(-1);
  }

  int temp = -1;
  lock_acquire(&required_lock);
  temp = filesys_remove(file);
  lock_release(&required_lock);
  f->eax = temp;
}
/*========================================================================================================================*/

/**
 * @brief Opens the file called file. Returns a nonnegative integer handle called a "file descriptor" (fd), or -1 if the file could not be opened.

 * File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is standard input, fd 1 (STDOUT_FILENO) is standard output. 
 * The open system call will never return either of these file descriptors, which are valid as system call arguments only as explicitly described below.
 * Each process has an independent set of file descriptors. File descriptors are not inherited by child processes.
 * When a single file is opened more than once, whether by a single process or different processes, each open returns a new file descriptor. 
 * Different file descriptors for a single file are closed independently in separate calls to close and they do not share a file position. 
 * 
 * @param f 
 */
void wrapper_sys_open(struct intr_frame *f) 
{
  char *file = (char *)(*((int *)f->esp + 1));
  if (!valid_in_virtual_memory(file))
  {
    sys_exit(-1);
  }
  // The curent_fd variable is used to assign a unique file descriptor every time a new file is opened. 
  // It's initialized to 2 because 0 and 1 are generally reserved for the standard input and output .
  static int curent_fd = 2;
  lock_acquire(&required_lock);
  struct file *file_obj = filesys_open(file);
  lock_release(&required_lock);

  if (file_obj == NULL)
  {
    f->eax = -1;
  }
  else
  {
    struct open_file *opened_file = (struct open_file *)malloc(sizeof(struct open_file));
    int file_fd = curent_fd;
    opened_file->fd   = curent_fd;
    opened_file->file = file_obj;

    lock_acquire(&required_lock);
    curent_fd++;
    lock_release(&required_lock);

    struct list_elem *elem = &opened_file->elem;
    list_push_back(&thread_current()->list_of_open_file, elem);
    f->eax = file_fd;
  }
}
/**
 * @brief Returns the size, in bytes, of the file open as fd.
 * 
 * @param f 
 */
void wrapper_sys_filesize(struct intr_frame *f) 
{
  int fd = (int)(*((int *)f->esp + 1));
  struct open_file *opened_file = get_file(fd);
  if (opened_file == NULL)
  {  f->eax = -1;
  }
  else
  {
    lock_acquire(&required_lock);
    f->eax = file_length(opened_file->file); // setting file size
    lock_release(&required_lock);
  }
}

/**
 * @brief Closes file descriptor fd. 
 * Exiting or terminating a process implicitly closes all its open file descriptors, as if by calling this function for each one. 
 * 
 * @param f 
 */
void wrapper_sys_close(struct intr_frame *f) {
  int fd = (int)(*((int *)f->esp + 1));
  // fd is nor stin or stout
  if (fd < 2)
  { 
    sys_exit(-1);
  }
  struct open_file *opened_file = get_file(fd);
  if (opened_file != NULL)
  {
    lock_acquire(&required_lock);
    file_close(opened_file->file);
    lock_release(&required_lock);
    list_remove(&opened_file->elem);
    f->eax = 1;
  }
  f->eax = -1;
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
  // not valid in its virtual memory or stout
  if (fd == 1 || !valid_in_virtual_memory(buffer))
  {
    sys_exit(-1);
  }
  else
  {
    unsigned int size = *((unsigned int *)f->esp + 3);
    if (fd == 0) 
    {
      // Read from the keyboard using input_getc()
      int i;
      for (i = 0; i < size; i++) {
        lock_acquire(&required_lock);
        char ch = input_getc();
        lock_release(&required_lock);
        buffer += ch;
      }
      f->eax = size;
    } 
    else {
      struct open_file *opened_file = get_file(fd);
      if (opened_file == NULL)
      {
        f->eax  = -1;
      }
      else
      {
        struct file *file = opened_file->file;      
        lock_acquire(&required_lock);
        f->eax = file_read(file, buffer, size);
        lock_release(&required_lock);
      }
    }
  }
}

/**
 * @brief Wrapper function for the `write` system call.
 *
 * Writes size bytes from bu er to the open le fd. Returns the number of bytes actually written, which may be less than size if some bytes could not be written. 
 * Writing past end-of-le would normally extend the le, but le growth is not implemented by the basic le system. The expected behavior is to write as many bytes as possible up to end-of-le and return the actual number written, or 0 if no bytes could be written at all.
 * Fd 1 writes to the console. Your code to write to the console should write all of bu er in one call to putbuf(), at least as long as size is not bigger than a few hundred bytes. (It is reasonable to break up largerbu ers.) Otherwise, lines of text output by di erent processes may endup interleaved on the console, confusing both human readers and ourgrading scripts.
 * @param f Pointer to the interrupt frame.
 */
void wrapper_sys_write(struct intr_frame *f)
{
  int fd = *((int *)f->esp + 1);
  char *buffer = (char *)(*((int *)f->esp + 2));
  if (fd == 0 || !valid_in_virtual_memory(buffer))
  { // fail, if fd is 0 (stdin), or its virtual memory
    sys_exit(-1);
  }
  unsigned size = (unsigned)(*((int *)f->esp + 3));

  if (fd == 1) {
    lock_acquire(&required_lock);
    putbuf(buffer, size);
    lock_release(&required_lock);
    f->eax = size;
  } 
  else 
  {
    struct open_file *opened_file = get_file(fd);
    if (opened_file == NULL) 
    {
      f->eax = -1;
    } 
    else 
    {
      lock_acquire(&required_lock);
      f->eax = file_write(opened_file->file, buffer, size);
      lock_release(&required_lock);
    }
  }
}

/**
 * @brief getting the file content of the desired opened file associated with the given file descriptor (fd).
 * 
 * @param fd The file descriptor of the file to retrieve. accessed by --> int fd = (int)(*((int *)f->esp + 1));
 * 
 * @return The open_file structure associated with the given file descriptor, or NULL if no such file is open.
 */
struct open_file *get_file(int fd)
{
  // accessing the list of opened files
  struct open_file *opened_file = NULL;
  struct list *list_of_files = &(thread_current()->list_of_open_file);
  struct list_elem *current = list_begin(list_of_files);
  // iterating over the list elemants to get the file content of the desired opened file
  while (current != list_end(list_of_files))
  {
    struct open_file *cur_file = list_entry(current, struct open_file, elem);
    if (cur_file->fd == fd)
    {
      opened_file = cur_file;
      break;
    }
    current = list_next(current);
  }
  return opened_file;
}



/*============================================NOTES==============================================*/

// esp is the stack pointer, which points to the top of the current stack frame.
// f->esp is a memory address where an integer value is stored. 
//  acess the value stored by :  1- (int *)f->esp         ==> pointer to an integer
//                               2- *((int *)f->esp)      ==> value of the integer
//                               3- *((int *)f->esp + 1)  ==> value of the integer at the next address

// Consider a function f() that takes three int arguments. 
// This diagram shows a sample stack frame as seen by the callee at the beginning of step 3 above, supposing that f() is invoked as f(1, 2, 3). 
// The initial stack address is arbitrary: 
//                              +----------------+
//                   0xbffffe7c |        3       |
//                   0xbffffe78 |        2       |
//                   0xbffffe74 |        1       |
// stack pointer --> 0xbffffe70 | return address |
//                              +----------------+

// status in f->esp and execute in f->eax
// int fd = (int)(*((int *)f->esp + 1));
//  fd --> file discreptor unique int for each file used for accessing the file 

/*=========================================HELPER FUNCTIONS====================================*/