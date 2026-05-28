//
// Created by yeonhyuk-kim on 2026. 5. 27.
//

#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stddef.h>

void swap_init();
size_t swap_out(void *kpage);
void swap_in(size_t slot, void *kpage);
void swap_free(size_t slot);

#endif //VM_SWAP_H
