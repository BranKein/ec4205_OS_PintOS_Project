#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void sys_halt (struct intr_frame *);
void sys_exit (struct intr_frame *);
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

void sys_halt (struct intr_frame *f) {}

void sys_exit (struct intr_frame *f) {
  int exit_code = *(int*)(f->esp + 4);
  thread_current()->exit_code = exit_code;
  thread_exit ();
}

void sys_wait (struct intr_frame *f) {}

void sys_create (struct intr_frame *f) {}

void sys_remove (struct intr_frame *f) {}

void sys_open (struct intr_frame *f) {}

void sys_filesize (struct intr_frame *f) {}

void sys_read (struct intr_frame *f) {}

void sys_write (struct intr_frame *f) {
  int fd = *(int*)(f->esp + 4);
  const void *buffer = *(void **)(f->esp + 8);
  unsigned size = *(unsigned*)(f->esp + 12);
  if (fd == 1) {
    putbuf(buf, size);
    f->eax = size;
  } else {
    f->eax = -1;
  }
}

void sys_seek (struct intr_frame *f) {}

void sys_tell (struct intr_frame *f) {}

void sys_close (struct intr_frame *f) {}