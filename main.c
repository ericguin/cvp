#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>

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

#define str_front(s) ((s).str[0])
#define str_back(s) ((s).str[(s).len - 1])

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

void str_scanner_pushtoken(str_scanner* scan, str token) {
  assert(token.str == (scan->base.str + (scan->cursor - token.len)));
  scan->cursor -= token.len;
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

#define X_PORT_DIRECTION() \
  XPDR(INPUT),\
  XPDR(OUTPUT),\
  XPDR(INOUT)

#define XPDR(n) PORT_DIRECTION_##n

typedef enum {
  X_PORT_DIRECTION()
} port_direction;

#undef XPDR
#define XPDR(n) STR_CONST(n)

str PORT_DIRECTION_NAMES[] = {
  X_PORT_DIRECTION()
};

#undef XPDR

size_t N_PORT_DIRECTION = sizeof(PORT_DIRECTION_NAMES) / sizeof(str);

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

#define X_SCOPE_TYPES() \
  XSTS(module), \
  XSTS(autofunction), \
  XSTS(function), \
  XSTS(autotask), \
  XSTS(task), \
  XSTS(begin), \
  XSTS(fork), \
  XSTS(autobegin), \
  XSTS(autofork), \
  XSTS(generate)

#define XSTS(t) SCOPE_TYPE_##t

typedef enum {
  X_SCOPE_TYPES()
} VPI_SCOPE_TYPE;

#undef XSTS

#define XSTS(t) STR_CONST(t)

str VPI_SCOPE_NAMES[] = {
  X_SCOPE_TYPES()
};

size_t N_VPI_SCOPE_TYPE = sizeof(VPI_SCOPE_NAMES)/sizeof(str);

#undef XSTS

typedef struct _vpi_scope {
  str name;
  size_t scope_id;
  str type_name;
  size_t line;
  str file;
  size_t def_line;
  str def_file;
  struct _vpi_scope* parent;
  timescale ts;
  port_info* ports;
  VPI_SCOPE_TYPE scope_type;
  bool is_cell;
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
  bool positive = str_equal(dir, STR_CONST(+)) ? true : false;

  mod->time_precision.precision = imag * (positive ? 1 : -1);

  printf("Found time precision: %d\n", mod->time_precision.precision);
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

size_t get_scope_id_from_str(str scopeid) {
  return strtoll(scopeid.str + 2, NULL, 0);
}

vvp_module parse_vvp_module(str bytecode, crena_arena* arena) {
  vvp_module ret = {0};

  // Header parsing
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

  crena_da_init(ret.scopes, arena);
  // Scope parsing
  // Stage 1: Find all scopes
  // and parse the basic information
  // Port info will be done in stage 2
  str_scanner ss1 = str_scanner_init(bytecode);
  while (str_scanner_more(ss1)) {
    // grab token
    str ident = str_scanner_nexttoken(&ss1);

    str type = str_scanner_nexttoken(&ss1);
    if (str_equal(type, STR_CONST(.scope))) {
      // we are a scope declaration
      vpi_scope scope = {0};
      str scopetype = str_scanner_nexttoken(&ss1);
      scope.name = str_scanner_nexttoken(&ss1);
      scope.type_name = str_scanner_nexttoken(&ss1);
      scope.scope_id = get_scope_id_from_str(ident);

      str sfile = str_scanner_nexttoken(&ss1);
      str sline = str_scanner_nexttoken(&ss1);

      scope.file = ret.file_names[atoi(sfile.str)];
      scope.line = atoi(sline.str);

      for (size_t i = 0; i < N_VPI_SCOPE_TYPE; i ++) {
        if (str_equal(scopetype, VPI_SCOPE_NAMES[i])) {
          scope.scope_type = (VPI_SCOPE_TYPE)i;
          break;
        }
      }

      if (str_back(sline) != ';') {
        // We are not a root scope
        str sdeffile = str_scanner_nexttoken(&ss1);
        str sdefline = str_scanner_nexttoken(&ss1);
        scope.def_file = ret.file_names[atoi(sdeffile.str)];
        scope.def_line = atoi(sdefline.str);

        str siscell = str_scanner_nexttoken(&ss1);

        scope.is_cell = atoi(siscell.str) > 0;

        str sparent = str_scanner_nexttoken(&ss1);

        size_t pid = get_scope_id_from_str(sparent);

        for (size_t i = 0; i < crena_da_len(ret.scopes); i ++) {
          if (ret.scopes[i].scope_id == pid) {
            scope.parent = &ret.scopes[i];
            break;
          }
        }
      }

      //TODO: Probably some defensive programming here
      str_scanner_nexttoken(&ss1);
      str smajor = str_scanner_nexttoken(&ss1);
      str sminor = str_scanner_nexttoken(&ss1);
      scope.ts.major = atoi(smajor.str);
      scope.ts.minor = atoi(sminor.str);


      str port_info_ident = str_scanner_nexttoken(&ss1);
      if (str_equal(port_info_ident, STR_CONST(.port_info))) {
        str_scanner_pushtoken(&ss1, port_info_ident);
        size_t ports_start = ss1.cursor;
        scope.ports = (port_info*)ports_start; // Cursed, but let's store the start location of ports in the pointer
      }

      crena_da_push(ret.scopes, scope);
    }

    str_scanner_skipuntil_nextline(&ss1);
  }

  crena_da_compress(ret.scopes);

  // Stage 2: Process port info on scopes
  for (size_t i = 0; i < crena_da_len(ret.scopes); i ++) {
    str_scanner scope_scan = str_scanner_init(bytecode);
    scope_scan.cursor = (size_t)ret.scopes[i].ports; // hehe un_curse
    crena_da_init(ret.scopes[i].ports, arena);
    if (scope_scan.cursor) {
      while (1) {
        str port_info_ident = str_scanner_nexttoken(&scope_scan);
        if (!str_equal(port_info_ident, STR_CONST(.port_info))) {
          str_scanner_pushtoken(&scope_scan, port_info_ident);
          break;
        }

        // TODO: find memory order of these guys
        str spnum = str_scanner_nexttoken(&scope_scan);
        str ptype = str_scanner_nexttoken(&scope_scan);
        str swidth = str_scanner_nexttoken(&scope_scan);
        str pname = str_scanner_nexttoken(&scope_scan);

        port_info inf = {0};
        inf.index = atoi(spnum.str);
        for (size_t pt = 0; pt < N_PORT_DIRECTION; pt ++) {
          if (str_equal(ptype, PORT_DIRECTION_NAMES[pt])) {
            inf.direction = (port_direction)pt;
          }
        }
        inf.width = atoi(swidth.str);
        inf.name = pname;
        crena_da_push(ret.scopes[i].ports, inf);
        str_scanner_skipuntil_nextline(&scope_scan);
      }
    }
    crena_da_compress(ret.scopes[i].ports);
  }

  for (size_t i = 0; i < crena_da_len(ret.scopes); i ++) {
    printf("Found a scope: %.*s\n", STR_PF(ret.scopes[i].name));
    printf("The scope has %ld ports\n", crena_da_len(ret.scopes[i].ports));
  }

  // Finally, let's grab the nets, vars, and functors

  return ret;
}

#ifndef UNIT_TEST

int main(int argc, char** argv) {
  crena_arena parse_arena = crena_init_growing();
  if (argc > 1) {
    str bytecode_str = read_entire_file(argv[1], &parse_arena);
    parse_vvp_module(bytecode_str, &parse_arena);
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
