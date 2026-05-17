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

bool is_valid_user_ptr(const void *ptr) {
  return ptr != NULL
    && is_user_vaddr(ptr)
    && pagedir_get_page(thread_current()->pagedir, ptr) != NULL;
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  if (!is_valid_user_ptr(f->esp)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

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
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int status = *(int*)(f->esp + 4);
  thread_current()->exit_code = status;
  thread_exit ();
}

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

void sys_wait (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int pid = *(int*)(f->esp + 4);
  int status = process_wait (pid);
  f->eax = status;
}

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