#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void sys_halt (struct intr_frame *);
void sys_exit (struct intr_frame *);
void sys_exec (struct intr_frame *);
void sys_wait (struct intr_frame *);
void sys_create (struct intr_frame *);
void sys_remove (struct intr_frame *);
void sys_open (struct intr_frame *);
void sys_filesize (struct intr_frame *);
void sys_read (struct intr_frame *);
void sys_write (struct intr_frame *);
void sys_seek (struct intr_frame *);
void sys_tell (struct intr_frame *);
void sys_close (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  // f->esp + 0: number of system call
  // f->esp + 4+: argument
  int syscall_num = *(int*)(f->esp);
  switch (syscall_num) {
    case SYS_HALT:
      sys_halt (f);
      break;
    case SYS_EXIT:
      sys_exit (f);
      break;
    case SYS_EXEC:
      sys_exec (f);
      break;
    case SYS_WAIT:
      sys_wait (f);
      break;
    case SYS_CREATE:
      sys_create (f);
      break;
    case SYS_REMOVE:
      sys_remove (f);
      break;
    case SYS_OPEN:
      sys_open (f);
      break;
    case SYS_FILESIZE:
      sys_filesize (f);
      break;
    case SYS_READ:
      sys_read (f);
      break;
    case SYS_WRITE:
      sys_write (f);
      break;
    case SYS_SEEK:
      sys_seek (f);
      break;
    case SYS_TELL:
      sys_tell (f);
      break;
    case SYS_CLOSE:
      sys_close (f);
      break;
    default:
      printf ("unknown system call %d\n", syscall_num);
      break;
  }
}

bool is_valid_user_ptr(const void *ptr) {
  return ptr != NULL
    && is_user_vaddr(ptr)
    && pagedir_get_page(thread_current()->pagedir, ptr) != NULL;
}

/*
 * System Call: void halt (void)
 * Terminates Pintos by calling shutdown_power_off() (declared in devices/shutdown.h).
 * This should be seldom used, because you lose some information about possible deadlock situations, etc.
 */
void sys_halt (struct intr_frame *f) {
  shutdown_power_off();
}

/*
 * System Call: void exit (int status)
 * Terminates the current user program, returning status to the kernel.
 * If the process's parent waits for it (see below), this is the status that will be returned. Conventionally, a status of 0 indicates success and nonzero values indicate errors.
 */
void sys_exit (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int status = *(int*)(f->esp + 4);
  thread_current()->exit_code = status;
  thread_exit ();
}

/*
 * System Call: pid_t exec (const char *cmd_line)
 * Runs the executable whose name is given in cmd_line, passing any given arguments, and returns the new process's program id (pid).
 * If the program cannot load or run for any reason, must return pid -1, which otherwise should not be a valid pid.
 * Thus, the parent process cannot return from the exec until it knows whether the child process successfully loaded its executable. You must use appropriate synchronization to ensure this.
 */
void sys_exec (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  const char *cmd_line = *(char**)(f->esp + 4);
  if (!is_valid_user_ptr(cmd_line)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  tid_t pid = process_execute (cmd_line);
  f->eax = pid;
}

/*
 * System Call: int wait (pid_t pid)
 * Waits for a child process pid and retrieves the child's exit status.
 *
 * If pid is still alive:
 * Wait until it terminates. Then, returns the status that pid passed to exit.
 * If pid did not call exit(), but was terminated by the kernel (e.g. killed due to an exception), wait(pid) must return -1.
 *
 * It is perfectly legal for a parent process to wait for child processes that have already terminated by the time the parent calls wait,
 * but the kernel must still allow the parent to retrieve its child's exit status,
 * or learn that the child was terminated by the kernel.
 *
 * 'wait' must fail and return -1 immediately if any of the following conditions is true:
 * 1. pid does not refer to a direct child of the calling process. pid is a direct child of the calling process if and only if the calling process received pid as a return value from a successful call to exec.
 * Note that children are not inherited: if A spawns child B and B spawns child process C, then A cannot wait for C, even if B is dead. A call to wait(C) by process A must fail.
 * Similarly, orphaned processes are not assigned to a new parent if their parent process exits before they do.
 * 2. The process that calls wait has already called wait on pid. That is, a process may wait for any given child at most once.
 *
 * Processes may spawn any number of children, wait for them in any order, and may even exit without having waited for some or all of their children.
 * Your design should consider all the ways in which waits can occur.
 * All of a process's resources, including its struct thread, must be freed whether its parent ever waits for it or not, and regardless of whether the child exits before or after its parent.
 *
 * You must ensure that Pintos does not terminate until the initial process exits.
 * The supplied Pintos code tries to do this by calling process_wait() (in userprog/process.c) from pintos_init() (in threads/init.c).
 * We suggest that you implement process_wait() according to the comment at the top of the function and then implement the wait system call in terms of process_wait().
 */
void sys_wait (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int pid = *(int*)(f->esp + 4);
  int status = process_wait (pid);
  f->eax = status;

  // TODO: handling exception without exit call?
}

/*
 * System Call: bool create (const char *file, unsigned initial_size)
 * Creates a new file called file initially initial_size bytes in size. Returns true if successful, false otherwise.
 * Creating a new file does not open it: opening the new file is a separate operation which would require a open system call.
 */
void sys_create (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4) || !is_valid_user_ptr(f->esp + 8)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  const char *file = *(char**)(f->esp + 4);
  if (!is_valid_user_ptr(file)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  unsigned initial_size = *(unsigned*)(f->esp + 8);
  f->eax = filesys_create (file, initial_size);
}

/*
 * System Call: bool remove (const char *file)
 * Deletes the file called file. Returns true if successful, false otherwise.
 * A file may be removed regardless of whether it is open or closed, and removing an open file does not close it. See Removing an Open File, for details.
 */
void sys_remove (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  const char *file = *(char**)(f->esp + 4);
  if (!is_valid_user_ptr(file)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  f->eax = filesys_remove (file);
}

/*
 *
 * System Call: int open (const char *file)
 * Opens the file called file. Returns a nonnegative integer handle called a "file descriptor" (fd), or -1 if the file could not be opened.
 * File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is standard input, fd 1 (STDOUT_FILENO) is standard output. The open system call will never return either of these file descriptors, which are valid as system call arguments only as explicitly described below.
 * Each process has an independent set of file descriptors. File descriptors are not inherited by child processes (different from Unix semantics)!!!
 * When a single file is opened more than once, whether by a single process or different processes, each open returns a new file descriptor. Different file descriptors for a single file are closed independently in separate calls to close and they do not share a file position.
 */
void sys_open (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  const char *file = *(char**)(f->esp + 4);
  if (!is_valid_user_ptr(file)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  // TODO: need lock for next_fd_i?
  struct file *of = filesys_open(file);
  if (of == NULL) {
    f->eax = -1;
    return;
  }

  int i = 2;
  for (i = 2; i < 128; i++) {
    if (thread_current()->fd_table[i] == NULL) {
      thread_current()->fd_table[i] = of;
      f->eax = i;
      return;
    }
  }
  file_close(of);
  f->eax = -1;
}

/*
 * System Call: int filesize (int fd)
 * Returns the size, in bytes, of the file open as fd.
 */
void sys_filesize (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int fd = *(int*)(f->esp + 4);

  if (fd < 2 || fd >= 128 || thread_current()->fd_table[fd] == NULL) {
    // not exists
    f->eax = -1;
    return;
  }
  struct file *fp = thread_current()->fd_table[fd];
  f->eax = file_length(fp);
}

/*
 * System Call: int read (int fd, void *buffer, unsigned size)
 *
 * Reads size bytes from the file open as fd into buffer.
 * Returns the number of bytes actually read (0 at end of file),
 * or -1 if the file could not be read (due to a condition other than end of file).
 *
 * Fd 0 reads from the keyboard using input_getc().
 */
void sys_read (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4) || !is_valid_user_ptr(f->esp + 8) || !is_valid_user_ptr(f->esp + 12)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int fd = *(int*)(f->esp + 4);
  const void *buffer = *(void **)(f->esp + 8);
  if (!is_valid_user_ptr(buffer)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  unsigned size = *(unsigned*)(f->esp + 12);
  if (fd == 0) {
    uint8_t *buf = (uint8_t *)buffer;
    unsigned i = 0;
    for (i = 0; i < size; i++) {
      buf[i] = input_getc();
    }
    f->eax = size;
  } else {
    if (fd < 2 || fd >= 128 || thread_current()->fd_table[fd] == NULL) {
      f->eax = -1;
      return;
    }
    struct file *fp = thread_current()->fd_table[fd];
    f->eax = file_read(fp, buffer, size);
  }
}

/*
 * System Call: int write (int fd, const void *buffer, unsigned size)
 * Writes size bytes from buffer to the open file fd. Returns the number of bytes actually written, which may be less than size if some bytes could not be written.
 * Writing past end-of-file would normally extend the file, but file growth is not implemented by the basic file system. The expected behavior is to write as many bytes as possible up to end-of-file and return the actual number written, or 0 if no bytes could be written at all.
 * Fd 1 writes to the console. Your code to write to the console should write all of buffer in one call to putbuf(), at least as long as size is not bigger than a few hundred bytes. (It is reasonable to break up larger buffers.) Otherwise, lines of text output by different processes may end up interleaved on the console, confusing both human readers and our grading scripts.
 */
void sys_write (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4) || !is_valid_user_ptr(f->esp + 8) || !is_valid_user_ptr(f->esp + 12)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int fd = *(int*)(f->esp + 4);
  const void *buffer = *(void **)(f->esp + 8);
  if (!is_valid_user_ptr(buffer)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  unsigned size = *(unsigned*)(f->esp + 12);
  if (fd == 1) {
    putbuf(buffer, size);
    f->eax = size;
  } else {
    if (fd < 2 || fd >= 128 || thread_current()->fd_table[fd] == NULL) {
      f->eax = -1;
      return;
    }
    struct file *fp = thread_current()->fd_table[fd];
    f->eax = file_write(fp, buffer, size);
  }
}

/*
 * System Call: void seek (int fd, unsigned position)
 *
 * Changes the next byte to be read or written in open file fd to position,
 * expressed in bytes from the beginning of the file. (Thus, a position of 0 is the file's start.)
 *
 * A seek past the current end of a file is not an error.
 * A later read obtains 0 bytes, indicating end of file.
 * A later write extends the file, filling any unwritten gap with zeros. (However, in Pintos files have a fixed length until project 4 is complete, so writes past end of file will return an error.)
 * These semantics are implemented in the file system and do not require any special effort in system call implementation.
 */
void sys_seek (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4) || !is_valid_user_ptr(f->esp + 8)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int fd = *(int*)(f->esp + 4);
  unsigned position = *(unsigned*)(f->esp + 8);

  if (fd < 2 || fd >= 128 || thread_current()->fd_table[fd] == NULL) {
    // not valid fd
    return;
  }
  struct file *fp = thread_current()->fd_table[fd];
  file_seek(fp, position);
}

/*
 * System Call: unsigned tell (int fd)
 * Returns the position of the next byte to be read or written in open file fd,
 * expressed in bytes from the beginning of the file.
 */
void sys_tell (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int fd = *(int*)(f->esp + 4);
  if (fd < 2 || fd >= 128 || thread_current()->fd_table[fd] == NULL) {
    // not valid fd
    return;
  }
  struct file *fp = thread_current()->fd_table[fd];
  f->eax = file_tell(fp);
}

/*
 * System Call: void close (int fd)
 *
 * Closes file descriptor fd.
 * Exiting or terminating a process implicitly closes all its open file descriptors,
 * as if by calling this function for each one.
 */
void sys_close (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int fd = *(int*)(f->esp + 4);
  if (fd < 2 || fd >= 128 || thread_current()->fd_table[fd] == NULL) {
    // not valid fd
    return;
  }
  struct file *fp = thread_current()->fd_table[fd];
  file_close(fp);
  thread_current()->fd_table[fd] = NULL;
}