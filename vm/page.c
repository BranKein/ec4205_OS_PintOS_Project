//
// Created by yeonhyuk-kim on 2026. 5. 27.
//

#include "page.h"
#include "swap.h"
#include <stdlib.h>

unsigned spt_hash_func(const struct hash_elem *e, void *aux);
bool spt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux);

void spt_init(struct hash *spt) {
  hash_init(spt, spt_hash_func, spt_less_func, NULL);
}

unsigned spt_hash_func(const struct hash_elem *e, void *aux) {
  struct spt_entry *spt_e = hash_entry (e, struct spt_entry, hash_elem);
  return hash_int((unsigned)spt_e->upage);
}

bool spt_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
  struct spt_entry *spt_a = hash_entry (a, struct spt_entry, hash_elem);
  struct spt_entry *spt_b = hash_entry (b, struct spt_entry, hash_elem);
  return spt_a->upage < spt_b->upage;
}

struct spt_entry* spt_find(struct hash *spt, void *upage) {
  struct spt_entry d;
  d.upage = upage;

  struct hash_elem *spt_e = hash_find(spt, &d.hash_elem);
  if (spt_e == NULL) return NULL;

  return hash_entry(spt_e, struct spt_entry, hash_elem);
}

void spt_insert(struct hash *spt, struct spt_entry *e) {
  hash_insert(spt, &e->hash_elem);
}

void spt_remove(struct hash *spt, void *upage) {
  struct spt_entry* e = spt_find(spt, upage);
  if (e == NULL) return;

  hash_delete(spt, &e->hash_elem);
  free(e);
}

static void spt_entry_free(struct hash_elem *e, void *aux) {
  struct spt_entry *entry = hash_entry(e, struct spt_entry, hash_elem);
  if (entry->type == PT_SWAP) {
    swap_free(entry->swap_slot);
  }
  free(entry);
}

void spt_clear(struct hash *spt) {
  hash_destroy(spt, spt_entry_free);
}