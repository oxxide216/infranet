#include "iui/iui.h"
#include "renderer.h"
#include "winx/event.h"
#include "shl_log.h"

static Iui iui = {0};

static bool process_event(IuiEvents *events, WinxEvent *event) {
  if (event->kind == WinxEventKindQuit) {
    return false;
  } else if (event->kind == WinxEventKindResize) {
    glass_resize(event->as.resize.width,
                 event->as.resize.height);
    iui_renderer_resize(&iui.renderer,
                      event->as.resize.width,
                      event->as.resize.height);
  } else {
    DA_APPEND(*events, *event);
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
  iui.renderer = iui_init_renderer();

  bool is_running = true;
  while (is_running) {
    iui.events.len = 0;

    WinxEvent event;
    while ((event = winx_get_event(&iui.window, false)).kind != WinxEventKindNone) {
      is_running = process_event(&iui.events, &event);
      if (!is_running)
        break;
    }

    EXECUTE_FUNC(vm, body.as.func.name, 0, false);

    Vec4 bounds = { 0.0, 0.0, iui.window.width, iui.window.height };
    iui_widgets_recompute_layout(&iui.widgets, bounds);
    iui_renderer_render_widgets(&iui.renderer, &iui.widgets);
    iui.widgets.is_dirty = false;

    winx_draw(&iui.window);
  }

  return true;
}

bool vbox_intrinsic(Vm *vm) {
  Value body = value_stack_pop(&vm->stack);
  if (body.kind != ValueKindFunc)
    PANIC("iui-vbox: wrong argument kind\n");

  iui_widgets_push_box_begin(&iui.widgets, vec2(25.0, 25.0),
                             IuiBoxDirectionVertical);

  EXECUTE_FUNC(vm, body.as.func.name, 0, false);

  iui_widgets_push_box_end(&iui.widgets);

  return true;
}

bool hbox_intrinsic(Vm *vm) {
  Value body = value_stack_pop(&vm->stack);
  if (body.kind != ValueKindFunc)
    PANIC("iui-hbox: wrong argument kind\n");

  iui_widgets_push_box_begin(&iui.widgets, vec2(25.0, 25.0),
                             IuiBoxDirectionHorizontal);

  EXECUTE_FUNC(vm, body.as.func.name, 0, false);

  iui_widgets_push_box_end(&iui.widgets);

  return true;
}

bool button_intrinsic(Vm *vm) {
  Value on_click = value_stack_pop(&vm->stack);
  Value text = value_stack_pop(&vm->stack);
  if (text.kind != ValueKindString ||
      on_click.kind != ValueKindFunc)
    PANIC("iui-button: wrong argument kinds\n");

  IuiWidget *button = iui_widgets_push_button(&iui.widgets,
                                              text.as.string,
                                              on_click.as.func);

  for (u32 i = 0; i < iui.events.len; ++i) {
    WinxEvent *event = iui.events.items + i;
    if (event->kind == WinxEventKindButtonRelease) {
      WinxEventButton *button_release = &event->as.button;

      if (button_release->x >= button->bounds.x &&
          button_release->y >= button->bounds.y &&
          button_release->x <= button->bounds.x + button->bounds.z &&
          button_release->y <= button->bounds.y + button->bounds.w) {
        EXECUTE_FUNC(vm, on_click.as.func.name, 0, false);
        break;
      }
    }
  }

  return true;
}

void iui_push_intrinsics(Intrinsics *intrinsics) {
  Intrinsic main_loop = {
    STR_LIT("iui-main-loop"), 4,
    false, &main_loop_intrinsic,
  };
  DA_APPEND(*intrinsics, main_loop);

  Intrinsic button = {
    STR_LIT("iui-button"), 2,
    false, &button_intrinsic,
  };
  DA_APPEND(*intrinsics, button);

  Intrinsic vbox = {
    STR_LIT("iui-vbox"), 1,
    false, &vbox_intrinsic,
  };
  DA_APPEND(*intrinsics, vbox);

  Intrinsic hbox = {
    STR_LIT("iui-hbox"), 1,
    false, &hbox_intrinsic,
  };
  DA_APPEND(*intrinsics, hbox);
}
