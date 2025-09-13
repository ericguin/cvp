#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#define CRENA_IMPLEMENTATION
#ifdef UNIT_TEST
#define CRENA_UT
#endif
#include "crena.h"

typedef struct {
  char const* str;
  size_t len;
} str;

typedef struct {
  str base;
  size_t cursor;
} str_scanner;

#define STR_PF(s) (int)s.len, s.str
#define STR_CONST(s) (str){ .str = #s , .len = sizeof(#s) - 1 }

bool str_equal(str a, str b) {
  if (a.len != b.len) return false;
  if (a.len == 0) return false;
  for (size_t i = 0; i < a.len; i ++) {
    if (a.str[i] != b.str[i]) return false;
  }
  return true;
}

str_scanner str_scanner_init(str input) {
  return (str_scanner) {
    .base = input,
    .cursor = 0
  };
}

#define str_scanner_front(scan) ((scan).base.str[(scan).cursor])
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

size_t str_scanner_skipwhile(str_scanner* scan, char skip) {
  size_t anchor = scan->cursor;
  while (str_scanner_more(*scan) && scan->base.str[scan->cursor] == skip) {
    scan->cursor++;
  }

  return scan->cursor - anchor;
}

size_t str_scanner_skipwhitespace(str_scanner* scan) {
  size_t anchor = scan->cursor;
  while (str_scanner_more(*scan) && isspace(scan->base.str[scan->cursor])) {
    scan->cursor++;
  }

  return scan->cursor - anchor;
}

str str_scanner_takeuntilwhitespace(str_scanner* scan) {
  size_t anchor = scan->cursor;
  while (str_scanner_more(*scan) && !isspace(scan->base.str[scan->cursor])) {
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

str str_scanner_nexttoken(str_scanner* scan) {
  str_scanner_skipwhitespace(scan);

  if (str_scanner_front(*scan) == '"') { // we have a quoted string!
    str_scanner_skipnext(scan);
    str ret = str_scanner_takeuntil(scan, '"');
    str_scanner_skipnext(scan);
    return ret;
  } else {
    return str_scanner_takeuntilwhitespace(scan);
  }
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

#define X_IVL_DELAY_SELECTION() \
  XIDS(TYPICAL),

#define XIDS(t) t

typedef enum {
  X_IVL_DELAY_SELECTION()
} IVL_DELAY_SELECTION;

#undef XIDS

#define XIDS(t) STR_CONST(t)

str IVL_DELAY_SELECTION_NAMES[] = {
  X_IVL_DELAY_SELECTION()
};

#undef XIDS

size_t N_IVL_DELAY_SELECTION = sizeof(IVL_DELAY_SELECTION_NAMES) / sizeof(str);

typedef struct {
  bool direction;
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

typedef struct {
  int32_t major;
  int32_t minor;
} timescale;

typedef struct {
  str scope_name;
  timescale ts;
  port_info* ports;
} vpi_scope;

typedef struct {
  ivl_version version;
  IVL_DELAY_SELECTION delay_selection;
  vpi_time_precision time_precision;
  str* file_names;
  vpi_scope* scopes;
} vvp_module;

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

#define IVLP_INI_FN(name) void ini_##name(vvp_module* mod, crena_arena* arena)
#define IVLP_FIN_FN(name) void fin_##name(vvp_module* mod, crena_arena* arena)
#define IVLP_PARSE_FN(name) void parse_##name(str_scanner* scan, vvp_module* mod, crena_arena* arena)

IVLP_PARSE_FN(ivl_version) {
  (void)arena;
  str build = str_scanner_nexttoken(scan);
  str hash = str_scanner_nexttoken(scan);

  // trim double quotes?
  mod->version.build = build;
  mod->version.hash = hash;

  printf("Set module build/version: %.*s | %.*s\n", STR_PF(build), STR_PF(hash));
}

IVLP_INI_FN(file_names) {
  crena_da_init(mod->file_names, arena);
}

IVLP_PARSE_FN(file_names) {
  (void)arena;

  str snum = str_scanner_nexttoken(scan); // Will include ;
  int num = atoi(snum.str);
  for (int i = 0; i < num; i++) {
    str fname = str_scanner_nexttoken(scan);
    str_scanner_skipuntil_nextline(scan);
    crena_da_push(mod->file_names, fname);
  }
}

IVLP_FIN_FN(file_names) {
  (void)arena;
  crena_da_compress(mod->file_names);
  for (size_t i = 0; i < crena_da_len(mod->file_names); i ++) {
    printf("File name: %.*s\n", STR_PF(mod->file_names[i]));
  }
}

IVLP_PARSE_FN(ivl_delay_selection) {
  (void)arena;
  str selection = str_scanner_nexttoken(scan);
  for (size_t i = 0; i < N_IVL_DELAY_SELECTION; i ++) {
    if (str_equal(selection, IVL_DELAY_SELECTION_NAMES[i])) {
      printf("Found delay selection: %.*s\n", STR_PF(IVL_DELAY_SELECTION_NAMES[i]));
      mod->delay_selection = (IVL_DELAY_SELECTION)i;
      break;
    }
  }
}

IVLP_PARSE_FN(vpi_time_precision) {
  (void)arena;

  str dir = str_scanner_nexttoken(scan);
  str mag = str_scanner_nexttoken(scan);

  int imag = atoi(mag.str);

  mod->time_precision.direction = str_equal(dir, STR_CONST(+)) ? true : false;
  mod->time_precision.precision = imag;

  printf("Found time precision: %d | %d\n", mod->time_precision.direction, mod->time_precision.precision);
}

struct {
  str ident_name;
  void(*ini_fn)(vvp_module*,crena_arena*);
  void(*parse_fn)(str_scanner*,vvp_module*,crena_arena*);
  void(*fin_fn)(vvp_module*,crena_arena*);
} ident_parsers[] = {
  {
    .ident_name = STR_CONST(ivl_version),
    .parse_fn = parse_ivl_version,
  },
  {
    .ident_name = STR_CONST(ivl_delay_selection),
    .parse_fn = parse_ivl_delay_selection,
  },
  {
    .ident_name = STR_CONST(vpi_time_precision),
    .parse_fn = parse_vpi_time_precision,
  },
  {
    .ident_name = STR_CONST(file_names),
    .ini_fn = ini_file_names,
    .fin_fn = fin_file_names,
    .parse_fn = parse_file_names,
  }
};

vvp_module bytecode_chunk_str(str bytecode, crena_arena* arena) {
  vvp_module ret = {0};

  for (size_t i = 0; i < sizeof(ident_parsers) / sizeof(ident_parsers[0]); i ++) {
    if (ident_parsers[i].ini_fn) ident_parsers[i].ini_fn(&ret, arena);

    if (ident_parsers[i].parse_fn) {
      str_scanner scan = str_scanner_init(bytecode);
      while (str_scanner_more(scan)) {
        char front = str_scanner_front(scan);

        if (front == ':') { // header entry
          str_scanner_skipnext(&scan); // skip :
          str identifier = str_scanner_nexttoken(&scan);

          if (str_equal(ident_parsers[i].ident_name, identifier)) {
            ident_parsers[i].parse_fn(&scan, &ret, arena);
          }
        }

        str_scanner_skipuntil_nextline(&scan);
      }
    }

    if (ident_parsers[i].fin_fn) ident_parsers[i].fin_fn(&ret, arena);
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
  (void)argc;
  (void)argv;
  printf("I am unit testing now!\n");
  crena_unit_test();
}

#endif
