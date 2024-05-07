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

// status in f->esp and execute in f->eax
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

int sys_create(const char *file, unsigned initial_size)
{
  int ans = 0;
  lock_acquire(&required_lock);
  ans = filesys_create(file, initial_size);
  lock_release(&required_lock);
  return ans;
}

int sys_remove(const char *file)
{
  int ans = -1;
  lock_acquire(&required_lock);
  ans = filesys_remove(file);
  lock_release(&required_lock);
  return ans;
}

int sys_open(const char *file)
{
  static unsigned long curent_fd = 2;
  lock_acquire(&required_lock);
  struct file *opened_file = filesys_open(file);
  lock_release(&required_lock);

  if (opened_file == NULL)
  { // fail
    return -1;
  }
  else
  {
    // open_file contains the file and file descriptor
    struct open_file *user_file = (struct open_file *)malloc(sizeof(struct open_file));
    int file_fd = curent_fd;
    user_file->fd = curent_fd;
    user_file->file = opened_file;

    lock_acquire(&required_lock);
    curent_fd++;
    lock_release(&required_lock);
    // list of opended files
    struct list_elem *elem = &user_file->elem;
    list_push_back(&thread_current()->list_of_open_file, elem);
    return file_fd;
  }
}

struct open_file *sys_file(int fd)
{
  struct open_file *ans = NULL;
  struct list *list_of_files = &(thread_current()->list_of_open_file);
  for (struct list_elem *cur = list_begin(list_of_files); cur != list_end(list_of_files); cur = list_next(cur))
  {
    struct open_file *cur_file = list_entry(cur, struct open_file, elem);
    if ((cur_file->fd) == fd)
    {
      return cur_file;
    }
  }
  return NULL;
}

int sys_read(int fd, void *buffer, unsigned size)
{
  int res = size;
  if (fd == 0)
  { // stdin .. 0 at end of file
    while (size--)
    {
      lock_acquire(&required_lock);
      char ch = input_getc();
      lock_release(&required_lock);
      buffer += ch;
    }
    return res;
  }

  struct open_file *user_file = sys_file(fd);
  if (user_file == NULL)
  { // fail
    return -1;
  }
  else
  {
    struct file *file = user_file->file;
    lock_acquire(&required_lock);
    size = file_read(file, buffer, size);
    lock_release(&required_lock);
    return size;
  }
}

int sys_write(int fd, const void *buffer, unsigned size)
{
  if (fd == 1)
  { // fd is 1, writes to the console
    lock_acquire(&required_lock);
    putbuf(buffer, size);
    lock_release(&required_lock);
    return size;
  }

  struct open_file *file = sys_file(fd);
  if (file == NULL)
  { // fail
    return -1;
  }
  else
  {
    int ans = 0;
    lock_acquire(&required_lock);
    ans = file_write(file->file, buffer, size);
    lock_release(&required_lock);
    return ans;
  }
}

void sys_seek(struct intr_frame *f)
{
  int fd = (int)(*((int *)f->esp + 1));
  unsigned pos = (unsigned)(*((int *)f->esp + 2));
  struct open_file *opened_file = sys_file(fd);
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

void sys_tell(struct intr_frame *f)
{
  int fd = (int)(*((int *)f->esp + 1));
  struct open_file *file = sys_file(fd);
  if (file == NULL)
  {
    f->eax = -1;
  }
  else
  {
    lock_acquire(&required_lock);
    f->eax = file_tell(file->file);
    lock_release(&required_lock);
  }
}

int sys_close(int fd)
{
  struct open_file *opened_file = sys_file(fd);
  if (opened_file != NULL)
  {
    lock_acquire(&required_lock);
    file_close(opened_file->file);
    lock_release(&required_lock);
    list_remove(&opened_file->elem);
    return 1;
  }
  return -1;
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

/* it's job is checking validation in virtual memory if valid pass it to sys_create() */
void wrapper_sys_create(struct intr_frame *f)
{
  char *file = (char *)*((int *)f->esp + 1);
  if (!valid_in_virtual_memory(file))
  {
    sys_exit(-1);
  }
  unsigned initial_size = (unsigned)*((int *)f->esp + 2);
  f->eax = sys_create(file, initial_size);
}

/* it's job is checking validation in virtual memory if valid pass it to sys_remove() */
void wrapper_sys_remove(struct intr_frame *f)
{
  char *file = (char *)(*((int *)f->esp + 1));
  if (!valid_in_virtual_memory(file))
  {
    sys_exit(-1);
  }
  f->eax = sys_remove(file);
}

/* it's job is checking validation in virtual memory if valid pass it to sys_open() */
void wrapper_sys_open(struct intr_frame *f)
{
  char *file = (char *)(*((int *)f->esp + 1));
  if (!valid_in_virtual_memory(file))
  {
    sys_exit(-1);
  }
  f->eax = sys_open(file);
}

/* it's job is checking validation in virtual memory if valid pass it to sys_file() */
void wrapper_sys_filesize(struct intr_frame *f)
{
  int fd = (int)(*((int *)f->esp + 1));
  struct open_file *file = sys_file(fd);
  if (file == NULL)
  { // fail
    f->eax = -1;
  }
  else
  {
    lock_acquire(&required_lock);
    f->eax = file_length(file->file); // setting file size
    lock_release(&required_lock);
  }
}
////////////////////////////////

/* it's job is checking validation in virtual memory if valid pass it to sys_read() */
void wrapper_sys_read(struct intr_frame *f)
{
  int fd = (int)(*((int *)f->esp + 1));
  char *buffer = (char *)(*((int *)f->esp + 2));
  if (fd == 1 || !valid_in_virtual_memory(buffer))
  { // fail if fd is 1 means (stdout) or in valid in virtual memory
    sys_exit(-1);
  }
  unsigned size = *((unsigned *)f->esp + 3);
  f->eax = sys_read(fd, buffer, size);
}

/* it's job is checking validation in virtual memory if valid pass it to sys_write() */
void wrapper_sys_write(struct intr_frame *f)
{
  int fd = *((int *)f->esp + 1);
  char *buffer = (char *)(*((int *)f->esp + 2));
  if (fd == 0 || !valid_in_virtual_memory(buffer))
  { // fail, if fd is 0 (stdin), or its virtual memory
    sys_exit(-1);
  }
  unsigned size = (unsigned)(*((int *)f->esp + 3));
  f->eax = sys_write(fd, buffer, size);
}

/* it's job is checking validation in virtual memory if valid pass it to sys_close() */
void wrapper_sys_close(struct intr_frame *f)
{
  int fd = (int)(*((int *)f->esp + 1));
  if (fd < 2)
  { // fail, the fd is stdin or stdout
    sys_exit(-1);
  }
  f->eax = sys_close(fd);
}

// Check validation of virtual memory :)
bool valid_in_virtual_memory(void *val)
{
  return val != NULL && is_user_vaddr(val) && pagedir_get_page(thread_current()->pagedir, val) != NULL;
}

// check validation of stack pointer
bool valid_esp(struct intr_frame *f)
{
  return valid_in_virtual_memory((int *)f->esp) || ((*(int *)f->esp) < 0) || (*(int *)f->esp) > 12;
}