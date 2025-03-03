#pragma once

#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

// https://github.com/olemorud/arena-allocator/tree/master
// I am going to take inspiration from that API because it is excellent
// TODO: add error checking
#define KNOB_MMAP_SIZE (1UL << 30UL)
#define KNOB_ALIGNMENT (sizeof(char*))

typedef enum flags {
  CRENA_ARENA_NOGROW = 1 << 0,
  CRENA_ARENA_NOALIGN = 1 << 1,
} crena_flags;

typedef struct _crena_arena {
  void *mem;
  size_t siz;
  size_t loc;
  crena_flags flags;
} crena_arena;

typedef enum {
  CRENA_FT_ALL,
  CRENA_FT_HOT_READY
} crena_free_type;

crena_arena crena_init(void*mem, size_t size);
crena_arena crena_init_growing();
void *crena_alloc(crena_arena *arena, size_t size);
void crena_free(crena_arena *arena, crena_free_type free_type);
void crena_dealloc(crena_arena *arena, size_t size);
size_t crena_amount_free(crena_arena *arena);
void crena_set_aigned(crena_arena* arena, bool aligned);

#define CRan(arena, type, count) crena_alloc(arena, sizeof(type) * count)
#define CRa(arena, type) CRan(arena, type, 1)

#define CRdn(arena, type, count) crena_dealloc(arena, sizeof(type) * count)
#define CRd(arena, type) CRdn(arena, type, 1)


#ifdef CRENA_IMPLEMENTATION

crena_arena crena_init(void *mem, size_t size) {
  crena_arena ret = {.mem = mem, .siz = size, .flags = 0b1};
  return ret;
}

crena_arena crena_init_growing() {
  size_t chunk_size = getpagesize();
  void *mem = mmap(NULL, KNOB_MMAP_SIZE, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  mprotect(mem, chunk_size, PROT_READ | PROT_WRITE);
  crena_arena ret = crena_init(mem, chunk_size);
  // enable growing
  ret.flags &= ~(0b1);

  return ret;
}

void crena_set_aigned(crena_arena* arena, bool aligned) {
  if (aligned) {
    arena->flags &= ~(0b10);
  } else {
    arena->flags |= 0b10;
  }
}

size_t crena_amount_free(crena_arena *arena) {
  return arena->siz - arena->loc;
}

static void _crena_grow(crena_arena *arena, size_t alloc_requested) {
  size_t page_size = getpagesize();
  size_t new_pages = (alloc_requested / page_size) + 1;
  size_t new_size = arena->siz + (new_pages * page_size);

  mprotect(arena->mem + arena->siz, new_size - arena->siz, PROT_READ | PROT_WRITE);
  arena->siz = new_size;
}

void *crena_alloc(crena_arena *arena, size_t size) {
  if ((arena->flags & 0b10) == 0) {
    size = (size + (KNOB_ALIGNMENT - 1)) &  ~(KNOB_ALIGNMENT - 1);
  }
  if (crena_amount_free(arena) > size) {
    void *ret = arena->mem + arena->loc;
    arena->loc += size;
    return ret;
  }
  if ((arena->flags & 0b1) == 0) {
    _crena_grow(arena, size);
    return crena_alloc(arena, size);
  }
  return NULL;
}

static void crena_free_all(crena_arena *arena) {
  munmap(arena->mem, KNOB_MMAP_SIZE);
}

void crena_free(crena_arena *arena, crena_free_type free_type) {
  if ((arena->flags & 0b1)) {
    arena->loc = 0;
  } else {
    switch (free_type) {
    case CRENA_FT_ALL:
      crena_free_all(arena);
      break;
    case CRENA_FT_HOT_READY:
      arena->loc = 0;
      break;
    }
  }
}

void crena_dealloc(crena_arena *arena, size_t size) {
  if (arena->loc - size >= 0) {
    arena->loc -= size;
  }
}

#endif
