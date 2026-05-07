#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <syscall.h>
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

static void syscall_handler(struct intr_frame *);
void check_valid_buffer (const void *buffer, unsigned size);
void check_valid_string(const void *str);
void check_valid_ptr(const void *vaddr);
void exit(int status);
void halt(void);
pid_t exec(const char *cmd_line);
int wait(pid_t pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);

// Files are NOT thread safe in PintOs (2 threads could edit the same file at the same time).
// So we define this lock, so each time a thread wants to access a file it must first aquire the lock
static struct lock fs_lock;

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");

  // Initialize the files lock
  lock_init(&fs_lock);
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  uint32_t *args = (uint32_t *)f->esp;
  check_valid_ptr(args);
  int sys_call_type = args[0];
  switch (sys_call_type)
  {
  case SYS_EXIT:
  {
    check_valid_buffer(args, 8);
    int status = (int)args[1];
    exit(status);
    break;
  }
  case SYS_HALT:
  {
    halt();
    break;
  }
  case SYS_EXEC:
  {
    check_valid_buffer(args, 8);
    char *cmd_line = (char *)args[1];
    check_valid_string(cmd_line);
    f->eax = exec(cmd_line);
    break;
  }
  case SYS_WAIT:
  {
    check_valid_buffer(args, 8);
    pid_t pid = (pid_t)args[1];
    f->eax = wait(pid);
    break;
  }
  case SYS_CREATE:
  {
    check_valid_buffer(args, 16);
    char *file = (char *)args[1];
    check_valid_string(file);
    unsigned initial_size = (unsigned)args[2];
    f->eax = create(file, initial_size);
    break;
  }
  case SYS_REMOVE:
  {
    check_valid_buffer(args, 8);
    char *file = (char *)args[1];
    check_valid_string(file);
    f->eax = remove(file);
    break;
  }
  case SYS_OPEN:
  {
    check_valid_buffer(args, 8);
    char *file = (char *)args[1];
    check_valid_string(file);
    f->eax = open(file);
    break;
  }
  case SYS_FILESIZE:
  {
    check_valid_buffer(args, 8);
    int fd = (int)args[1];
    f->eax = filesize(fd);
    break;
  }
  case SYS_READ:
  {
    check_valid_buffer(args, 16);
    int fd = (int)args[1];
    void *buffer = (void *)args[2];
    check_valid_ptr(buffer);
    unsigned size = (unsigned)args[3];
    check_valid_buffer(buffer, size);
    f->eax = read(fd, buffer, size);
    break;
  }
  case SYS_WRITE:
  {
    check_valid_buffer(args, 16);
    int fd = (int)args[1];
    void *buffer = (void *)args[2];
    check_valid_ptr(buffer);
    unsigned size = (unsigned)args[3];
    check_valid_buffer(buffer, size);
    f->eax = write(fd, buffer, size);
    break;
  }
  case SYS_SEEK:
  {
    check_valid_buffer(args, 12);
    int fd = (int)args[1];
    unsigned position = (unsigned)args[2];
    seek(fd, position);
    break;
  }
  case SYS_TELL:
  {
    check_valid_buffer(args, 8);
    int fd = (int)args[1];
    f->eax = tell(fd);
    break;
  }
  case SYS_CLOSE:
  {
    check_valid_buffer(args, 8);
    int fd = (int)args[1];
    close(fd);
    break;
  }
  default:
    exit(-1);
    break;
  }
}

void exit(int status)
{
  struct thread* current = thread_current();

  // Print very epic and cool exit msg
  printf("%s: exit(%d)\n", current->name, status);

  thread_exit();
}

void halt(void)
{
  shutdown_power_off();
}

pid_t exec(const char *cmd_line)
{
  // TODO
}

int wait(pid_t pid)
{
  // TODO
}

// Just a wrapper around filesys_create
bool create(const char *file, unsigned initial_size)
{
  lock_acquire(&fs_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&fs_lock);
  return success;
}

// Just a wrapper around filesys_remove
bool remove(const char *file)
{
  lock_acquire(&fs_lock);
  bool success = filesys_remove(file);
  lock_release(&fs_lock);
  return success;
}

int open(const char *file)
{
  // Notice how here we use a char* (string) instead of an int fd
  // This is because when we want to open a file we supply the fn with the file name
  // This open function takes the file name and searches for it in the disk using filesys_open
  // We then take the file and put it in the fdt so that in the other operations we don't have to;
  // search for the file in the disk again, but instead we could just use the int fd.


  lock_acquire(&fs_lock);
  struct file* opened_file = filesys_open(file); // Search for file in disk

  if (opened_file == NULL)
  {
    lock_release(&fs_lock);
    return -1;
  }

  // Find an empty slot in the FDT (starting at 2, since 1 and 0 are reserved for STDIN and STDOUT)
  struct thread* t = thread_current();
  for (int i = 2; i < 128; i++)
  {
    if (t->fdt[i] == NULL)
    {
      t->fdt[i] = opened_file;
      lock_release(&fs_lock);
      return i; // We return the fd to the user (index in the fdt)
    }
  }

  // If table is full
  file_close(opened_file);
  lock_release(&fs_lock);
  return -1;
}

// Wrapper around file_length
int filesize(int fd)
{
  if (fd < 2 || fd >= 128) return -1;

  lock_acquire(&fs_lock);
  struct file* f = thread_current()->fdt[fd];

  if (f == NULL)
  {
    lock_release(&fs_lock);
    return -1;
  }

  int len = file_length(f);
  lock_release(&fs_lock);
  return len;
}

int read(int fd, void *buffer, unsigned size)
{

  // Out of range
  if (fd < 0 || fd >= 128) return -1;

  // fs == 1 is STDOUT
  // You can't read from STDOUT
  if (fd == 1) return -1;

  // fd == 0 is STDIN
  // STDIN is usually the keyboard
  if (fd == 0)
  {

    // Buffer is void*
    // We want to store chars in buffer so we cast it to 8bit pointer
    uint8_t *local_buffer = (uint8_t *) buffer;

    // Read chars from STDIN
    for (int i = 0; i < size; i++) {
      // input_getc() waits for and reads a char
      // defined in devices/input.h
      local_buffer[i] = input_getc();
    }

    return size;
  }

  lock_acquire(&fs_lock);
  struct file *f = thread_current()->fdt[fd];

  if (f == NULL)
  {
    lock_release(&fs_lock);
    return -1;
  }

  int bytes_read = file_read(f, buffer, size);
  lock_release(&fs_lock);
  return bytes_read;
}

int write(int fd, const void *buffer, unsigned size)
{
  /*
   *
   * Return Value:
   *    If success: return number of bytes written
   *    If fail: return -1
   *
   * One might consider this edge case: What if a thread attempts to write on its own;
   * executable file?!
   * Well gladly for us this case is handled in the file_write() function that we call below;
   * so there is no need for us to handle it ourselves
   */

  // Out of range
  if (fd < 0 || fd >= 128) return -1;

  // fd == 1 is STDOUT
  if (fd == 1)
  {
    // This fn is the one used by the kernal to actually write to STDOUT
    // Defined in lib/kernal/stdio.h
    putbuf(buffer, size);
    return size;
  }

  // fd == 0 is STDIN
  // You can't write to input!
  if (fd == 0) return -1;

  lock_acquire(&fs_lock);

  // Retrieve the file that we want to write to
  struct file *f = thread_current()->fdt[fd];

  // Failed to read file
  if (f == NULL)
  {
    lock_release(&fs_lock);
    return -1;
  }
  int bytes_written = file_write(f, buffer, size);
  lock_release(&fs_lock);
  return bytes_written;
}

// Jump to a specific byte in the file
void seek(int fd, unsigned position)
{
  if (fd < 2 || fd >= 128) return;

  lock_acquire(&fs_lock);
  struct file *f = thread_current()->fdt[fd];
  if (f != NULL)
  {
    file_seek(f, position);
  }
  lock_release(&fs_lock);
}

// Tells where we are in the file crrently
// Similar to ur cursor in the file, where we are editing rn
unsigned tell(int fd)
{
  if (fd < 2 || fd >= 128) return 0;

  lock_acquire(&fs_lock);
  struct file *f = thread_current()->fdt[fd];
  unsigned pos = 0;
  if (f != NULL)
  {
    pos = file_tell(f);
  }
  lock_release(&fs_lock);
  return pos;
}

void close(int fd)
{

  // OUT of range
  // Can't close STDIN or STDOUT
  if (fd < 2 || fd >= 128) return;


  lock_acquire(&fs_lock);
  if (thread_current()->fdt[fd] != NULL) {
    file_close(thread_current()->fdt[fd]);
    thread_current()->fdt[fd] = NULL;
  }
  lock_release(&fs_lock);
}

// This is a helper method that should be called when a thread/process;
// finishes executing as to ensure that all it's files are closed
void close_all_files()
{
  for (int i = 2; i < 128; i++)
  {
    close(i);
  }
}

void check_valid_buffer (const void *buffer, unsigned size) 
{
  if (size == 0)
  {
    return;
  }
  const char *current = (const char *) buffer;
  const char *end = current + size - 1;
  check_valid_ptr(current);
  // 1. Fast-forward our pointer to the start of the NEXT memory page
  current = (const char *) pg_round_down(current) + PGSIZE;
  // 2. Jump forward exactly one page at a time (4096 bytes) 
  // until we pass the end of the buffer
  while (current <= end) 
  {
    check_valid_ptr(current);
    current += PGSIZE;
  }

}
void check_valid_string(const void *str){
  check_valid_ptr(str);
  char *ptr = (char *) str;
  // While we haven't hit the end of the string...
  while (*ptr != '\0') 
  {
    ptr++; 
    // If the next character crosses a page boundary, validate the new page
    if ((uint32_t) ptr % PGSIZE == 0) {
      check_valid_ptr((const void *) ptr);
    }
  }
}

void check_valid_ptr(const void *vaddr){
  // 1. Check if null
  // 2. Check if it points to kernel memory (above PHYS_BASE)
  if (vaddr == NULL || !is_user_vaddr(vaddr))
  {
    exit(-1);
  }
  
  // 3. Check if the page is actually mapped in the page directory
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (ptr == NULL) {
    exit(-1);
  } 
}

