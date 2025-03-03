#pragma once
#include "types.h"

#define _LIST_T_IMPL(type) \
  void type##_list_push(type##_list* list, type item) {\
    list->items[list->count] = item;\
    list->count ++;\
    if (list->count >= list->capacity) {\
      crena_dealloc(list->arena, sizeof(type) * list->capacity);\
      list->capacity *= 2;\
      crena_alloc(list->arena, sizeof(type) * list->capacity);\
    }\
  }\
  type##_list type##_list_init_capacity(crena_arena* arena, size_t capacity) {\
    void* mem = crena_alloc(arena, sizeof(type) * capacity);\
    type##_list ret = {\
      .arena = arena,\
      .capacity = 20,\
      .items = mem\
    };\
    return ret;\
  }\
  type##_list type##_list_init(crena_arena* arena) {\
    return type##_list_init_capacity(arena, 20);\
  }\
  void type##_list_shrink(type##_list* list) {\
    size_t diff = list->capacity - list->count;\
    list->capacity = list->count;\
    crena_dealloc(list->arena, diff * sizeof(type));\
  }\
  type* type##_list_get(type##_list* list, size_t idx) {\
    if (idx > list->count) return NULL;\
    return &list->items[idx];\
  }

_LIST_T_IMPL(port_info);
_LIST_T_IMPL(str);
