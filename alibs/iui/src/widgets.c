#include "widgets.h"
#include "shl_defs.h"

static void iui_widget_recompute_layout(IuiWidget *widget, Vec4 bounds, bool *is_dirty) {
  if (!*is_dirty)
    *is_dirty = widget->bounds.x != bounds.x ||
                widget->bounds.y != bounds.y ||
                widget->bounds.z != bounds.z ||
                widget->bounds.w != bounds.w;

  widget->bounds = bounds;

  if (widget->kind != IuiWidgetKindBox ||
      widget->as.box.children.len == 0 ||
      !*is_dirty)
    return;

  INFO("Layout was recomputed!\n");

  Vec4 child_bounds = {
    bounds.x + widget->as.box.margin.x,
    bounds.y + widget->as.box.margin.y,
    bounds.z - widget->as.box.margin.x * 2.0,
    bounds.w - widget->as.box.margin.y * 2.0,
  };

  if (child_bounds.w < 0.0)
    child_bounds.w = 0.0;
  if (child_bounds.z < 0.0)
    child_bounds.z = 0.0;

  if (widget->as.box.direction == IuiBoxDirectionVertical) {
    child_bounds.w -= (widget->as.box.children.len - 1) * widget->as.box.margin.y;
    child_bounds.w /= widget->as.box.children.len;
  } else {
    child_bounds.z -= (widget->as.box.children.len - 1) * widget->as.box.margin.x;
    child_bounds.z /= widget->as.box.children.len;
  }

  for (u32 i = 0; i < widget->as.box.children.len; ++i) {
    iui_widget_recompute_layout(widget->as.box.children.items[i],
                                child_bounds, is_dirty);

    if (widget->as.box.direction == IuiBoxDirectionVertical)
      child_bounds.y += child_bounds.w + widget->as.box.margin.y;
    else
      child_bounds.x += child_bounds.z + widget->as.box.margin.x;
  }
}

void iui_widgets_recompute_layout(IuiWidgets *widgets, Vec4 bounds) {
  iui_widget_recompute_layout(widgets->root_widget, bounds,
                              &widgets->is_dirty);
}

static void iui_widget_reset_layout(IuiWidget *widget) {
  if (widget->kind == IuiWidgetKindBox) {
    for (u32 i = 0; i < widget->as.box.children.len; ++i)
      iui_widget_reset_layout(widget->as.box.children.items[i]);

    widget->as.box.children.len = 0;
  }
}

void iui_widgets_reset_layout(IuiWidgets *widgets) {
  iui_widget_reset_layout(widgets->root_widget);

  widgets->is_dirty = false;
}

static IuiWidget *iui_widgets_get_widget(IuiWidgets *widgets,
                                         char *file_path,
                                         u32 line) {
  IuiWidget *result = NULL;

  IuiWidget *widget = widgets->list;
  while (widget) {
    if (strcmp(widget->file_path, file_path) == 0 &&
        widget->line == line) {
      result = widget;
      break;
    }

    widget = widget->next;
  }

  if (!result) {
    IuiWidget widget = {0};
    widget.file_path = file_path;
    widget.line = line;
    LL_PREPEND(widgets->list, widgets->list_end, IuiWidget);
    *widgets->list_end = widget;
    result = widgets->list_end;
  }

  if (widgets->boxes.len > 0) {
    IuiChildren *children = &widgets->boxes.items[widgets->boxes.len - 1]->children;
    DA_APPEND(*children, result);
  } else {
    widgets->root_widget = result;
  }

  return result;
}

IuiWidget *iui_widgets_push_box_begin_id(IuiWidgets *widgets, Vec2 margin,
                                         IuiBoxDirection direction,
                                         char *file_path, u32 line) {
  IuiWidget *widget = iui_widgets_get_widget(widgets, file_path, line);
  widget->kind = IuiWidgetKindBox;
  widget->as.box.margin = margin;
  widget->as.box.direction = direction;
  widget->as.box.children = (IuiChildren) {0};

  DA_APPEND(widgets->boxes, &widget->as.box);

  return widget;
}

void iui_widgets_push_box_end(IuiWidgets *widgets) {
  if (widgets->boxes.len > 0)
    --widgets->boxes.len;
}

IuiWidget *iui_widgets_push_button_id(IuiWidgets *widgets, Str text,
                                      ValueFunc on_click,
                                      char *file_path, u32 line) {
  IuiWidget *widget = iui_widgets_get_widget(widgets, file_path, line);
  widget->kind = IuiWidgetKindButton;
  widget->as.button.text = text;
  widget->as.button.on_click = on_click;

  return widget;
}
