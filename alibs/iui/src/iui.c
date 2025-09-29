#include "iui/iui.h"
#include "winx/event.h"
#include "glass/glass.h"

static Iui iui = {0};

void iui_init(Str window_title,
              u32 window_width,
              u32 window_height) {
  iui.winx = winx_init();
  iui.window = winx_init_window(&iui.winx, window_title,
                                window_width, window_height,
                                WinxGraphicsModeOpenGL,
                                NULL);
  glass_init();
}

static bool process_event(WinxEvent *event) {
  if (event->kind == WinxEventKindQuit) {
    return false;
  } else if (event->kind == WinxEventKindResize) {
    glass_resize(event->as.resize.width,
                 event->as.resize.height);
  }

  return true;
}

bool iui_main_loop_intrinsic(Vm *vm) {
  Value body = value_stack_pop(&vm->stack);
  if (body.kind != ValueKindFunc)
    PANIC("iui-main-loop: wrong argument kind\n");



  bool is_running = true;
  while (is_running) {
    WinxEvent event;
    while ((event = winx_get_event(&iui.window, false)).kind != WinxEventKindNone) {
      is_running = process_event(&event);
      if (!is_running)
        break;
    }

    EXECUTE_FUNC(vm, body.as.func.name, 0, false);

    glass_clear_screen(0.0, 0.0, 0.0, 0.5);
    winx_draw(&iui.window);
  }

  return true;
}

void iui_push_intrinsics(Intrinsics *intrinsics) {
  Intrinsic main_loop = {
    STR_LIT("iui-main-loop"), 1,
    false, &iui_main_loop_intrinsic,
  };
  DA_APPEND(*intrinsics, main_loop);
}
