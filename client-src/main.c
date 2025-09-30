#include "aether-vm/vm.h"
#include "aether-ir/deserializer.h"
#ifndef NONET
#include "net.h"
#else
#include "io.h"
#endif
#ifdef IUI
#include "iui/iui.h"
#endif
#include "shl_defs.h"
#include "shl_log.h"
#define SHL_STR_IMPLEMENTATION
#include "shl_str.h"
#define SHL_ARENA_IMPLEMENTATION
#include "shl_arena.h"

#ifdef IUI
#define WINDOW_TITLE STR_LIT("Infranet client")
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#endif

int main(i32 argc, char **argv) {
#ifndef NONET
  char *route = "";
  if (argc > 1)
    route = argv[1];

  Str bytecode = get_bytecode_from_server("127.0.0.1", 8080, route);
#else
  if (argc < 2) {
    ERROR("No input file were provided\n");
    return 1;
  }

  Str bytecode = read_file(argv[1]);
#endif

  RcArena rc_arena = {0};
  Ir ir = deserialize((u8 *) bytecode.ptr, bytecode.len, &rc_arena);
  Intrinsics intrinsics = {0};

#ifdef IUI
  iui_push_intrinsics(&intrinsics);
#endif

  execute(&ir, argc, argv, &rc_arena, &intrinsics);

  free(bytecode.ptr);

  return 0;
}
