#pragma once

#include "crena.h"
#include <stdint.h>
#include <stddef.h>

#define _LIST_T(type) typedef struct _##type##_list {\
  type* items;\
  size_t count;\
  size_t capacity;\
  crena_arena* arena;\
} type##_list;

#define _LL_T(type)\
  typedef struct _##type##_ll_item {\
    type item;\
    struct _##type##_ll_item* next;\
    struct _##type##_ll_item* prev;\
  } type##_ll_item;\
  typedef struct _##type##_ll {\
    type##_ll_item* head;\
    type##_ll_item* base;\
    crena_arena* arena;\
  } type##_ll;

typedef struct {
  char const* str;
  size_t len;
} str;

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

typedef struct {
  str module_path;
} vpi_module;

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

typedef struct {
  str_list names;
} file_names;

typedef struct {
  ivl_version version;
  ivl_delay_selection delay_selection;
  vpi_time_precision time_precision;
  file_names files;
} vvp_module;

typedef enum {
  NONE,
} vvp_toplevel_type;

typedef struct {
  vvp_toplevel_type type;
  uint64_t id;
  union {
    ivl_version version;
    ivl_delay_selection delay_selection;
    vpi_time_precision time_precision;
    vpi_scope scope;
  } declaration;
} vvp_toplevel_declaration;
