//
// Created by yeonhyuk-kim on 2026. 5. 27.
//

#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "filesys/file.h"
#include "threads/vaddr.h"

enum page_type {
  PT_FILE, // page that has to be read from elf executable file
  PT_ZERO, // page that has been filled by 0x00, BSS segments
  PT_SWAP, // page in swap, that had been in physical memory, then gone to swap (disk)
};

// struct for supplementary page table
struct spt_entry {
  uint8_t *upage;
  enum page_type type;
  bool writable;

  // if PT_FILE
  struct file *file;
  off_t ofs;
  uint32_t read_bytes;
  uint32_t zero_bytes;

  // if PT_SWAP
  size_t swap_slot;

  // for store in hash table
  struct hash_elem hash_elem;
};

void spt_init(struct hash *spt);
struct spt_entry* spt_find(struct hash *spt, void *upage);
void spt_insert(struct hash *spt, struct spt_entry *e);
void spt_remove(struct hash *spt, void *upage);
void spt_clear(struct hash *spt);

#endif //VM_PAGE_H
