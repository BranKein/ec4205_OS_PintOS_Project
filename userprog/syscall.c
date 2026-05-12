#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"

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

void sys_halt (struct intr_frame *f) {
  shutdown_power_off();
}

void sys_exit (struct intr_frame *f) {
  int status = *(int*)(f->esp + 4);
  thread_current()->exit_code = status;
  thread_exit ();
}

void sys_exec (struct intr_frame *f) {
  const char *cmd_line = *(char**)(f->esp + 4);
  tid_t pid = process_exec (cmd_line);
  f->eax = pid;

  // TODO: the parent process cannot return from the exec until it knows whether the child process successfully loaded its executable. You must use appropriate synchronization to ensure this.
}

void sys_wait (struct intr_frame *f) {
  int pid = *(int*)(f->esp + 4);
  int status = process_wait (pid);
  f->eax = status;

  // TODO: handling exception without exit call?
}

void sys_create (struct intr_frame *f) {
  const char *file = *(char**)(f->esp + 4);
  unsigned initial_size = *(unsigned*)(f->esp + 8);
  f->eax = filesys_create (file, initial_size);
}

void sys_remove (struct intr_frame *f) {
  const char *file = *(char**)(f->esp + 4);
  f->eax = filesys_remove (file);
}

void sys_open (struct intr_frame *f) {
  const char *file = *(char**)(f->esp + 4);
  // TODO: return type mismatch
  // f->eax = filesys_open (file);
}

void sys_filesize (struct intr_frame *f) {
  int fd = *(int*)(f->esp + 4);
  // TODO: check!
  // f->eax = filesys_filesize (fd);
}

void sys_read (struct intr_frame *f) {
  int fd = *(int*)(f->esp + 4);
  const void *buffer = *(void **)(f->esp + 8);
  unsigned size = *(unsigned*)(f->esp + 12);
  if (fd == 0) {
    input_getc(buffer, size);
    f->eax = size;
  } else {
    f->eax = -1;
  }
}

void sys_write (struct intr_frame *f) {
  int fd = *(int*)(f->esp + 4);
  const void *buffer = *(void **)(f->esp + 8);
  unsigned size = *(unsigned*)(f->esp + 12);
  if (fd == 1) {
    putbuf(buffer, size);
    f->eax = size;
  } else {
    f->eax = -1;
  }
}

void sys_seek (struct intr_frame *f) {
  int fd = *(int*)(f->esp + 4);
  unsigned position = *(unsigned*)(f->esp + 8);
  // TODO: ??
}

void sys_tell (struct intr_frame *f) {
  int fd = *(int*)(f->esp + 4);
  // TODO: check!
  // filesys_tell(fd);
}

void sys_close (struct intr_frame *f) {
  int fd = *(int*)(f->esp + 4);
  // TODO: check!
  // filesys_close(fd);
}