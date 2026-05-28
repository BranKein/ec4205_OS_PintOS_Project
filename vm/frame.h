//
// Created by yeonhyuk-kim on 2026. 5. 27.
//

#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "threads/synch.h"
#include <list.h>

struct lock frame_lock;

struct frame_entry {
  void *kpage;
  void *upage;
  struct thread *t;
  struct list_elem elem;
};

void frame_table_init();
void *frame_alloc(enum palloc_flags flags, void *upage);
void frame_free(void *kpage);

#endif //VM_FRAME_H
