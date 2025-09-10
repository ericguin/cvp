#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#define CRENA_IMPLEMENTATION
#include "crena.h"

#define _LIST_T(type) typedef struct _##type##_list {\
  type* items;\
  size_t count;\
  size_t capacity;\
  crena_arena* arena;\
} type##_list;

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

typedef struct {
  char const* str;
  size_t len;
} str;

typedef struct {
  str base;
  size_t cursor;
} str_scanner;

#define STR_PF(s) (int)s.len, s.str

str_scanner str_scanner_init(str input) {
  return (str_scanner) {
    .base = input,
    .cursor = 0
  };
}

#define str_scanner_front(scan) (scan.base.str[scan.cursor])
#define str_scanner_more(scan) ((scan).cursor <= (scan).base.len)

str str_scanner_takeuntil(str_scanner* scan, char needle) {
  size_t anchor = scan->cursor;
  while (str_scanner_more(*scan) && scan->base.str[scan->cursor] != needle) {
    scan->cursor++;
  }

  return (str) {
    .str = &scan->base.str[anchor],
    .len = scan->cursor - anchor
  };
}

bool str_scanner_skipnext(str_scanner* scan) {
  if (str_scanner_more(*scan)) {
    scan->cursor++;
    return true;
  }

  return false;
}

str str_scanner_takeuntil_nextline(str_scanner* scan) {
  str ret = str_scanner_takeuntil(scan, '\n');
  str_scanner_skipnext(scan);
  return ret;
}

size_t str_scanner_skipuntil(str_scanner* scan, char needle) {
  return str_scanner_takeuntil(scan, needle).len;
}

size_t str_scanner_skipuntil_nextline(str_scanner* scan) {
  return str_scanner_takeuntil_nextline(scan).len;
}

typedef struct {
  str build;
  str hash;
} ivl_version;

typedef enum {
  TYPICAL,
} IVL_DELAY_SELECTION;

typedef struct {
  IVL_DELAY_SELECTION selection;
} ivl_delay_selection;

typedef struct {
  int32_t precision;
} vpi_time_precision;

typedef enum {
  SIGNAL_TYPE_NET,
  SIGNAL_TYPE_VAR
} signal_type;

typedef struct {
  str name;
  int32_t start;
  int32_t end;
  int64_t driver_id;
} signal;

typedef enum {
  PORT_DIRECTION_INPUT,
  PORT_DIRECTION_OUTPUT,
  PORT_DIRECTION_INOUT
} port_direction;

typedef struct {
  size_t index;
  str name;
  int32_t width;
  port_direction direction;
} port_info;

_LIST_T(port_info);
_LIST_T(str);

typedef struct {
  int32_t major;
  int32_t minor;
} timescale;

typedef struct {
  str scope_name;
  timescale ts;
  port_info_list ports;
} vpi_scope;

_LIST_T(vpi_scope);

typedef struct {
  ivl_version version;
  ivl_delay_selection delay_selection;
  vpi_time_precision time_precision;
  str_list file_names;
  vpi_scope_list scopes;
} vvp_module;


_LIST_T_IMPL(port_info);
_LIST_T_IMPL(str);
_LIST_T_IMPL(vpi_scope);


str read_entire_file(char const* filename, crena_arena* arena) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    perror("Failed to open file");
    str result = {NULL, 0};
    return result;
  }

  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)crena_alloc(arena, file_size + 1);
  if (!buffer) {
    perror("Failed to allocate memory");
    fclose(file);
    str result = {NULL, 0};
    return result;
  }

  size_t read_size = fread(buffer, 1, file_size, file);
  if (read_size != file_size) {
    perror("Failed to read file");
    crena_dealloc(arena, file_size);
    fclose(file);
    str result = {NULL, 0};
    return result;
  }

  buffer[file_size] = '\0';
  fclose(file);
  str result = {buffer, file_size};
  return result;
}

vvp_module bytecode_chunk_str(str bytecode, crena_arena* arena) {
  (void)arena;
  vvp_module ret = {0};
  str_scanner scan = str_scanner_init(bytecode);

  while (str_scanner_more(scan)) {
    char front = str_scanner_front(scan);

    switch (front) {
      case ':': // header entry
        str_scanner_skipnext(&scan);
        str identifier = str_scanner_takeuntil(&scan, ' ');
        printf("Identifier is: %.*s\n", STR_PF(identifier));
        break;
      default:
        break;
    }

    str nextline = str_scanner_takeuntil_nextline(&scan);
  }

  return ret;
}

#ifndef UNIT_TEST

int main(int argc, char** argv) {
  crena_arena parse_arena = crena_init_growing();
  if (argc > 1) {
    str bytecode_str = read_entire_file(argv[1], &parse_arena);
    bytecode_chunk_str(bytecode_str, &parse_arena);
  }
}

#else // All code below is UT

int main(int argc, char** argv) {
  printf("I am unit testing now!\n");
}

#endif
