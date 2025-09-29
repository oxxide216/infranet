#include "iui/iui.h"
#include "winx/event.h"
#include "glass/glass.h"

static Iui iui = {0};

static bool process_event(WinxEvent *event) {
  if (event->kind == WinxEventKindQuit) {
    return false;
  } else if (event->kind == WinxEventKindResize) {
    glass_resize(event->as.resize.width,
                 event->as.resize.height);
  }

  return true;
}

bool main_loop_intrinsic(Vm *vm) {
  Value body = value_stack_pop(&vm->stack);
  Value height = value_stack_pop(&vm->stack);
  Value width = value_stack_pop(&vm->stack);
  Value title = value_stack_pop(&vm->stack);
  if (title.kind != ValueKindString ||
      width.kind != ValueKindInt ||
      height.kind != ValueKindInt ||
      body.kind != ValueKindFunc)
    PANIC("iui-main-loop: wrong argument kinds\n");

  iui.winx = winx_init();
  iui.window = winx_init_window(&iui.winx, title.as.string,
                                width.as._int, height.as._int,
                                WinxGraphicsModeOpenGL,
                                NULL);

  glass_init();
  bool is_running = true;
  while (is_running) {
    WinxEvent event;
    while ((event = winx_get_event(&iui.window, false)).kind != WinxEventKindNone) {
      is_running = process_event(&event);
      if (!is_running)
        break;
    }

    EXECUTE_FUNC(vm, body.as.func.name, 0, false);

    winx_draw(&iui.window);
  }

  return true;
}

void iui_push_intrinsics(Intrinsics *intrinsics) {
  Intrinsic main_loop = {
    STR_LIT("iui-main-loop"), 4,
    false, &main_loop_intrinsic,
  };
  DA_APPEND(*intrinsics, main_loop);
}
