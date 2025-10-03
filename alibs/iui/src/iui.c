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

    Record window_size = {0};

    NamedValue width = {
      STR_LIT("x"),
      {
        ValueKindFloat,
        { ._float = iui.window.width },
      }
    };
    DA_APPEND(window_size, width);

    NamedValue height = {
      STR_LIT("y"),
      {
        ValueKindFloat,
        { ._float =  iui.window.height },
      }
    };
    DA_APPEND(window_size, height);

    value_stack_push_record(&vm->stack, window_size);

    EXECUTE_FUNC(vm, body.as.func.name, 1, false);

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
  Value spacing = value_stack_pop(&vm->stack);
  Value margin_y = value_stack_pop(&vm->stack);
  Value margin_x = value_stack_pop(&vm->stack);

  if (margin_x.kind != ValueKindFloat ||
      margin_y.kind != ValueKindFloat ||
      spacing.kind != ValueKindFloat ||
      body.kind != ValueKindFunc)
    PANIC("iui-vbox: wrong argument kinds\n");

  iui_widgets_push_box_begin(&iui.widgets, vec2(margin_x.as._float, margin_y.as._float),
                             spacing.as._float, IuiBoxDirectionVertical);

  EXECUTE_FUNC(vm, body.as.func.name, 0, false);

  iui_widgets_push_box_end(&iui.widgets);

  return true;
}

bool hbox_intrinsic(Vm *vm) {
  Value body = value_stack_pop(&vm->stack);
  Value spacing = value_stack_pop(&vm->stack);
  Value margin_y = value_stack_pop(&vm->stack);
  Value margin_x = value_stack_pop(&vm->stack);
  if (margin_x.kind != ValueKindFloat ||
      margin_y.kind != ValueKindFloat ||
      spacing.kind != ValueKindFloat ||
      body.kind != ValueKindFunc)
    PANIC("iui-hbox: wrong argument kinds\n");

  iui_widgets_push_box_begin(&iui.widgets, vec2(margin_x.as._float, margin_y.as._float),
                             spacing.as._float, IuiBoxDirectionHorizontal);

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

bool abs_bounds_intrinsic(Vm *vm) {
  Value height = value_stack_pop(&vm->stack);
  Value width = value_stack_pop(&vm->stack);
  Value y = value_stack_pop(&vm->stack);
  Value x = value_stack_pop(&vm->stack);
  if (x.kind != ValueKindFloat ||
      y.kind != ValueKindFloat ||
      width.kind != ValueKindFloat ||
      height.kind != ValueKindFloat)
    PANIC("iui-abs: wrong argument kinds\n");

  Vec4 bounds = {
    x.as._float,
    y.as._float,
    width.as._float,
    height.as._float,
  };
  iui_widgets_abs_bounds(&iui.widgets, bounds);

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
    STR_LIT("iui-vbox"), 4,
    false, &vbox_intrinsic,
  };
  DA_APPEND(*intrinsics, vbox);

  Intrinsic hbox = {
    STR_LIT("iui-hbox"), 4,
    false, &hbox_intrinsic,
  };
  DA_APPEND(*intrinsics, hbox);

  Intrinsic abs_bounds = {
    STR_LIT("iui-abs-bounds"), 4,
    false, &abs_bounds_intrinsic,
  };
  DA_APPEND(*intrinsics, abs_bounds);
}
