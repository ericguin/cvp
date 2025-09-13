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

#ifdef CRENA_UT
#define CRPF(...) printf(__VA_ARGS__)
#else
#define CRPF(...)
#endif

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
void *crena_realloc(crena_arena *arena, void* mem, size_t oldsiz, size_t newsiz);
void crena_free(crena_arena *arena, crena_free_type free_type);
void crena_dealloc(crena_arena *arena, size_t size);
size_t crena_amount_free(crena_arena *arena);
void crena_set_aligned(crena_arena* arena, bool aligned);

typedef struct {
  size_t count;
  size_t capacity;
  crena_arena* arena;
} _crena_da_header;

void* _crena_da_init(size_t esize, crena_arena* arena);
void* _crena_da_grow(void* daptr, size_t size, size_t count);

#define crena_da_header(da) ((_crena_da_header*)(da) - 1)
#define crena_da_init(da, arena) (da) = _crena_da_init(sizeof(*da), arena);
#define crena_da_push(da, itm) ((da) = _crena_da_grow(da, sizeof(*da), 1), (da)[crena_da_header(da)->count++] = (itm))
#define crena_da_pop(da) (crena_da_header(da)->count--, (da)[crena_da_header(da)->count])
#define crena_da_len(da) (crena_da_header(da)->count)

#define CRan(arena, type, count) crena_alloc(arena, sizeof(type) * count)
#define CRa(arena, type) CRan(arena, type, 1)

#define CRdn(arena, type, count) crena_dealloc(arena, sizeof(type) * count)
#define CRd(arena, type) CRdn(arena, type, 1)


#ifdef CRENA_IMPLEMENTATION

void* _crena_da_init(size_t esize, crena_arena* arena) {
#ifdef CRENA_UT
  static size_t capacity = 2;
#else
  static size_t capacity = 20;
#endif
  size_t alloc_size = esize * capacity + sizeof(_crena_da_header);
  char* mem = crena_alloc(arena, alloc_size);
  char* ret = mem + sizeof(_crena_da_header);
  _crena_da_header* header = (_crena_da_header*)mem;
  header->arena = arena;
  header->count = 0;
  header->capacity = capacity;
  return ret;
}

void* _crena_da_grow(void* daptr, size_t size, size_t count) {
  _crena_da_header* header = crena_da_header(daptr);
  if (header->count + count <= header->capacity) return daptr;

  size_t actual_size = header->capacity * size + sizeof(_crena_da_header);
  size_t actual_new_size = (header->capacity * 2) * size + sizeof(_crena_da_header);
  char* mem = crena_realloc(header->arena, header, actual_size, actual_new_size);
  char* ret = mem + sizeof(_crena_da_header);
  return ret;
}

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

void crena_set_aligned(crena_arena* arena, bool aligned) {
  if (aligned) {
    arena->flags &= ~(0b10);
  } else {
    arena->flags |= 0b10;
  }
}

size_t crena_amount_free(crena_arena *arena) {
  return arena->siz - arena->loc;
}

void *crena_realloc(crena_arena *arena, void* mem, size_t oldsiz, size_t newsiz) {
  char* cmem = (char*)mem;
  char* amem = ((char*)arena->mem) + arena->loc;

  bool can_grow_without_copy = cmem == (amem - oldsiz);

  if (can_grow_without_copy) {
    crena_dealloc(arena, oldsiz);
    return crena_alloc(arena, newsiz);
  } else {
    void* ret = crena_alloc(arena, newsiz);
    memcpy(ret, mem, oldsiz);
    return ret;
  }
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
  if (crena_amount_free(arena) >= size) {
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
  if (size >= arena->loc) {
    arena->loc -= size;
  }
}

#endif

#ifdef CRENA_UT

void crena_unit_test() {
  int* da;
  crena_arena arena = crena_init_growing();

  crena_da_init(da, &arena);

  crena_da_push(da, 1);
  crena_da_push(da, 2);
  crena_da_push(da, 3);

  printf("Size of da is: %ld\n", crena_da_len(da));

  for (size_t i = 0; i < crena_da_len(da); i ++) {
    printf("At %ld : %d\n", i, da[i]);
  }

  printf("If I grab the last: %d\n", crena_da_pop(da));
  printf("What is my new length? %ld\n", crena_da_len(da));
}

#endif
