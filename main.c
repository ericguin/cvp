#define CRENA_IMPLEMENTATION
#include "crena.h"
#include "types.h"
#include "lists.c"
#include "bytecode.c"
#include <stdio.h>

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

int main(int argc, char** argv) {
  crena_arena parse_arena = crena_init_growing();
  if (argc > 1) {
    str bytecode_str = read_entire_file(argv[1], &parse_arena);
    bytecode_chunk_str(bytecode_str);
  }
}
