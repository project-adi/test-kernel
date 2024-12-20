#include <mm/heap.h>
#include <lib/libc.h>
#include <dev/char/serial.h>

heap* heap_create(pagemap* pm) {
  heap* h = (heap*)HIGHER_HALF(pmm_alloc(1));
  h->block_head = (heap_block*)HIGHER_HALF(pmm_alloc(1));
  h->block_head->magic = HEAP_MAGIC;
  h->block_head->next = h->block_head->prev = h->block_head;
  h->block_head->size = 0;
  h->block_head->state = 1;
  h->pm = pm;
  return h;
}

void* heap_alloc(heap* h, u64 size) {
  lock(&h->hl);
  u64 pages = DIV_ROUND_UP(sizeof(heap_block) + size, PAGE_SIZE);
  u8* buf = HIGHER_HALF(pmm_alloc(pages));
  heap_block* block = (heap_block*)buf;
  block->magic = HEAP_MAGIC;
  block->size = size;
  block->state = 1;
  block->prev = h->block_head->prev;
  block->next = h->block_head;
  h->block_head->prev->next = block;
  h->block_head->prev = block;
  unlock(&h->hl);
  return buf + sizeof(heap_block);
}

void heap_free(heap* h, void* ptr) {
  lock(&h->hl);
  heap_block* block = (heap_block*)(ptr - sizeof(heap_block));
  if (block->magic != HEAP_MAGIC) {
    dprintf("heap_free(): Invalid magic at pointer %lx.\n", ptr);
    unlock(&h->hl);
    return;
  }
  block->prev->next = block->next;
  block->next->prev = block->prev;
  u64 pages = DIV_ROUND_UP(sizeof(heap_block) + block->size, PAGE_SIZE);
  u8* buf = (u8*)(ptr - sizeof(heap_block));
  buf = PHYSICAL(buf);
  pmm_free(buf, pages);
  unlock(&h->hl);
}

void* heap_realloc(heap* h, void* ptr, u64 size) {
  heap_block* block = (heap_block*)(ptr - sizeof(heap_block));
  if (block->magic != HEAP_MAGIC) {
    dprintf("heap_realloc(): Invalid magic at pointer %lx.\n", ptr);
    return NULL;
  }
  void* new_ptr = heap_alloc(h, size);
  if (!new_ptr)
    return NULL;
  memcpy(new_ptr, ptr, block->size);
  heap_free(h, ptr);
  return new_ptr;
}

uptr heap_get_allocation_paddr(heap* h, uptr ptr) {
  heap_block* block = (heap_block*)(ptr - sizeof(heap_block));
  if (block->magic != HEAP_MAGIC)
    return 0;
  return vmm_get_region_paddr(h->pm, (ptr - sizeof(heap_block)));
}

void heap_clone(heap* h, heap* clone) {
  // Here we set the clone block head to the original block head,
  // which contains the linked list of allocations!

  clone->block_head = h->block_head;
}

void heap_destroy(heap* h) {
  for (heap_block* block = h->block_head->next; block != h->block_head; block = block->next) {
    u8* buf = (u8*)block;
    vma_region* region = vmm_find_range(h->pm, (uptr)buf);
    if (region->ref_count >= 1) {
      vmm_free(h->pm, (void*)region->vaddr, region->pages);
    }
  }
}
