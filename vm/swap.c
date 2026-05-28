//
// Created by yeonhyuk-kim on 2026. 5. 27.
//

#include "swap.h"

#include <bitmap.h>

#include "../lib/kernel/bitmap.h"
#include "devices/block.h"
#include "threads/vaddr.h"

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

static struct block *swap_block;
static struct bitmap *swap_bitmap;

void swap_init() {
  swap_block = block_get_role(BLOCK_SWAP);
  swap_bitmap = bitmap_create(block_size(swap_block) / SECTORS_PER_PAGE);
}

size_t swap_out (void *kpage) {
  // find empty slot
  size_t slot = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);
  int i;
  for (i = 0; i < SECTORS_PER_PAGE; i++) {
    block_write(swap_block, slot * SECTORS_PER_PAGE + i, kpage + i * BLOCK_SECTOR_SIZE);
  }
  return slot;
}

void swap_in(size_t slot, void *kpage) {
  int i;
  for (i = 0; i < SECTORS_PER_PAGE; i++) {
    block_read(swap_block, slot * SECTORS_PER_PAGE + i, kpage + i * BLOCK_SECTOR_SIZE);
  }
  bitmap_flip(swap_bitmap, slot);
}

void swap_free(size_t slot) {
  bitmap_flip(swap_bitmap, slot);
}
