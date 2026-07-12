#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/page.h"

static void syscall_handler (struct intr_frame *);

struct lock filesys_lock;

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
void sys_mmap (struct intr_frame *);
void sys_munmap (struct intr_frame *);

bool is_valid_user_ptr(const void *ptr) {
  // return ptr != NULL
    // && is_user_vaddr(ptr)
    // && pagedir_get_page(thread_current()->pagedir, ptr) != NULL;
  if (ptr == NULL || !is_user_vaddr(ptr))
    return false;
  struct thread *ct = thread_current();
  void *pg = pagedir_get_page(ct->pagedir, ptr);
  if (pg != NULL) return true;
  if (spt_find(&ct->spt, pg_round_down((void*)ptr)) != NULL) return true;
  
  return (uintptr_t)ptr >= (uintptr_t)PHYS_BASE - (8 * 1024 * 1024);
}


void
syscall_init (void) 
{
  lock_init (&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  thread_current()->user_esp = f->esp;
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
    case SYS_MMAP:
      sys_mmap (f);
      break;
    case SYS_MUNMAP:
      sys_munmap (f);
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
  lock_acquire (&filesys_lock);
  f->eax = filesys_create (file, initial_size);
  lock_release (&filesys_lock);
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

  lock_acquire (&filesys_lock);
  f->eax = filesys_remove (file);
  lock_release (&filesys_lock);
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

  lock_acquire (&filesys_lock);
  struct file *of = filesys_open(file);
  lock_release (&filesys_lock);
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
  lock_acquire (&filesys_lock);
  file_close(of);
  lock_release (&filesys_lock);
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
  lock_acquire (&filesys_lock);
  f->eax = file_length(fp);
  lock_release (&filesys_lock);
}

/*
 * when sys read or write, it calls file_read/write, it acquires IDE lock internally.
 * then try to access user buffer, but if the buffer is in swap, page fault occurs,
 * while handling page fault, try to swap in, it tries to acquire IDE lock again.
 *
 * therefore, with prefault_buffer func, we try to read buffer first then occurs page fault,
 * load buffer in the memory before calling file_read/write.
 */
void prefault_buffer(const void *buffer, unsigned size) {
  uint8_t *b = (uint8_t *)buffer;
  unsigned i;
  for (i = 0; i < size; i += PGSIZE) {
    volatile uint8_t tmp = b[i];
    (void)tmp;
  }
  if (size > 0) {
    volatile uint8_t tmp = b[size - 1];
    (void)tmp;
  }
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
    prefault_buffer(buffer, size);
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
    prefault_buffer(buffer, size);
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
  lock_acquire (&filesys_lock);
  file_seek(fp, position);
  lock_release (&filesys_lock);
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
  lock_acquire (&filesys_lock);
  f->eax = file_tell(fp);
  lock_release (&filesys_lock);
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
  lock_acquire (&filesys_lock);
  file_close(fp);
  lock_release (&filesys_lock);
  thread_current()->fd_table[fd] = NULL;
}

void sys_mmap (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4) || !is_valid_user_ptr(f->esp + 8)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int fd = *(int*)(f->esp + 4);
  if (fd < 2 || fd >= 128 || thread_current()->fd_table[fd] == NULL) {
    // not valid fd
    f->eax = -1;
    return;
  }
  struct file *fp = thread_current()->fd_table[fd];

  void *addr = *(void **)(f->esp + 8);

  lock_acquire (&filesys_lock);
  size_t fl = file_length(fp);
  if (fl == 0) {
    f->eax = -1;
    lock_release (&filesys_lock);
    return;
  }

  if (addr == NULL || !is_user_vaddr(addr) || (uintptr_t)addr % PGSIZE != 0) {
    f->eax = -1;
    lock_release (&filesys_lock);
    return;
  }

  struct thread *ct = thread_current();
  struct hash *cur_spt = &ct->spt;

  size_t page_cnt = (fl + PGSIZE - 1) / PGSIZE;

  size_t i;
  for (i = 0; i < page_cnt; i++) {
    void *upage = (uint8_t*)addr + i * PGSIZE;
    if (spt_find(cur_spt, upage) != NULL) {
      f->eax = -1;
      lock_release (&filesys_lock);
      return;
    }
  }

  struct file *mmap_file = file_reopen(fp); // reopen the file for separation

  for (i = 0; i < page_cnt; i++) {
    void *upage = (uint8_t*)addr + i * PGSIZE;
    struct spt_entry *spt_e = malloc(sizeof *spt_e);
    spt_e->upage = upage;
    spt_e->type = PT_MMAP;
    spt_e->writable = true;

    spt_e->file = mmap_file;
    spt_e->ofs = i * PGSIZE;

    uint32_t read_bytes = (i + 1) * PGSIZE <= fl ? PGSIZE : fl - (off_t)(i * PGSIZE);
    uint32_t zero_bytes = PGSIZE - read_bytes;
    spt_e->read_bytes = read_bytes;
    spt_e->zero_bytes = zero_bytes;
    spt_insert(cur_spt, spt_e);
  }

  struct mmap_entry *mmap_e = malloc(sizeof *mmap_e);
  int mapid = ct->next_mapid++;
  mmap_e->mapid = mapid;
  mmap_e->file = mmap_file;
  mmap_e->addr = addr;
  mmap_e->page_cnt = page_cnt;
  list_push_back(&ct->mmap_list, &mmap_e->elem);

  lock_release (&filesys_lock);

  f->eax = mapid;
}

void sys_munmap (struct intr_frame *f) {
  if (!is_valid_user_ptr(f->esp + 4)) {
    thread_current()->exit_code = -1;
    thread_exit();
  }

  int mapping = *(int*)(f->esp + 4); // mapid_t

  struct list_elem *e = list_begin (&thread_current()->mmap_list);
  while (e != list_end (&thread_current()->mmap_list)) {
    struct mmap_entry *mmap_e = list_entry (e, struct mmap_entry, elem);
    struct list_elem *next = list_next (e);
    if (mmap_e->mapid == mapping) {
      // dirty write-back
      list_remove (e);
      munmap_clear(mmap_e);
    }
    e = next;
  }

}
