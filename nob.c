#define NOB_IMPLEMENTATION
#include "nob.h"

Nob_Cmd cmd = {0};

int main(int argc, char **argv)
{
  NOB_GO_REBUILD_URSELF(argc, argv);

  nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-o", "main", "main.c", "-ggdb");
  if (!nob_cmd_run(&cmd)) return 1;

  nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-o", "ut", "main.c", "-ggdb", "-DUNIT_TEST");
  if (!nob_cmd_run(&cmd)) return 1;

  return 0;
}
