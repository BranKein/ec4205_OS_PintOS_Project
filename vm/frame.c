//
// Created by yeonhyuk-kim on 2026. 5. 27.
//

#include "frame.h"

#include <stdlib.h>

#include "page.h"
#include "swap.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

static struct list frame_table_list;
static struct lock frame_lock;

struct frame_entry* evict_frame();

void frame_table_init() {
  list_init (&frame_table_list);
  lock_init(&frame_lock);
}

void* frame_alloc(enum palloc_flags flags, void *upage) {
  lock_acquire(&frame_lock);

  // try palloc_get_page
  uint8_t *kpage = palloc_get_page (flags);

  // if success, add frame_entry, return kpage
  if (kpage != NULL) {
    struct frame_entry *fe = malloc(sizeof *fe);
    fe->kpage = kpage;
    fe->upage = upage;
    fe->t = thread_current();
    list_push_back(&frame_table_list, &fe->elem);

    lock_release(&frame_lock);
    return kpage;
  }

  // if fail, select victim frame then swap out
  struct frame_entry* victim_fe = evict_frame();
  if (victim_fe == NULL) {
    lock_release(&frame_lock);
    return NULL;
  }

  struct thread* victim_t = victim_fe->t;
  pagedir_clear_page(victim_t->pagedir, victim_fe->upage);

  struct spt_entry* victim_spt = spt_find(&victim_t->spt, victim_fe->upage);
  if (victim_spt == NULL) {
    frame_free(victim_fe->kpage);
    lock_release(&frame_lock);
    return NULL;
  }


  // if dirty or anonymous, swap out
  if (pagedir_is_dirty(victim_t->pagedir, victim_fe->upage) || victim_spt->type != PT_FILE) {
    // call swap_out
    victim_spt->type = PT_SWAP;
    victim_spt->swap_slot = swap_out(victim_fe->kpage);
  }
  frame_free(victim_fe->kpage);

  // retry palloc_get_page
  kpage = palloc_get_page (flags);
  // if fail, return NULL
  if (kpage == NULL) {
    lock_release(&frame_lock);
    return NULL;
  }

  struct frame_entry *fe = malloc(sizeof *fe);
  fe->kpage = kpage;
  fe->upage = upage;
  fe->t = thread_current();
  list_push_back(&frame_table_list, &fe->elem);

  lock_release(&frame_lock);
  return kpage;
}

void frame_free(void *kpage) {
  lock_aquire(&frame_lock);

  struct list_elem *e;
  for (e = list_begin(&frame_table_list); e != list_end(&frame_table_list); e = list_next(e)) {
    struct frame_entry *fe = list_entry(e, struct frame_entry, elem);
    if (fe->kpage == kpage) {
      list_remove(e);
      free(fe);
      palloc_free_page(kpage);
      lock_release(&frame_lock);
      return;
    }
  }
  lock_release(&frame_lock);
}

struct frame_entry* evict_frame() {
  // find frame that will be the victim then swap out
  struct list_elem *e;
  int i;
  for (i = 0; i < 2; i++) {
    for (e = list_begin(&frame_table_list); e != list_end(&frame_table_list); e = list_next(e)) {
      struct frame_entry *fe = list_entry(e, struct frame_entry, elem);
      if (pagedir_is_accessed(fe->t->pagedir, fe->upage)) {
        pagedir_set_accessed(fe->t->pagedir, fe->upage, false);
      } else {
        return fe;
      }
    }
  }
  return NULL;
}