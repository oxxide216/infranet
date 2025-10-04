#include "widgets.h"
#include "shl_defs.h"

static void iui_widget_recompute_layout(IuiWidget *widget, Vec4 bounds, bool *is_dirty) {
  if (!*is_dirty)
    *is_dirty = widget->bounds.x != bounds.x ||
                widget->bounds.y != bounds.y ||
                widget->bounds.z != bounds.z ||
                widget->bounds.w != bounds.w;

  if (!*is_dirty)
    return;

  if (!widget->use_abs_bounds)
    widget->bounds = bounds;

  if (widget->kind != IuiWidgetKindBox || widget->as.box.children.len == 0)
    return;

  Vec4 child_bounds = {
    widget->bounds.x + widget->as.box.margin.x,
    widget->bounds.y + widget->as.box.margin.y,
    widget->bounds.z - widget->as.box.margin.x * 2.0,
    widget->bounds.w - widget->as.box.margin.y * 2.0,
  };

  if (widget->as.box.direction == IuiBoxDirectionVertical) {
    child_bounds.w -= (widget->as.box.children.len - 1) * widget->as.box.spacing;
    child_bounds.w /= widget->as.box.children.len;
  } else {
    child_bounds.z -= (widget->as.box.children.len - 1) * widget->as.box.spacing;
    child_bounds.z /= widget->as.box.children.len;
  }

  if (child_bounds.w < 0.0)
    child_bounds.w = 0.0;
  if (child_bounds.z < 0.0)
    child_bounds.z = 0.0;

  for (u32 i = 0; i < widget->as.box.children.len; ++i) {
    iui_widget_recompute_layout(widget->as.box.children.items[i],
                                child_bounds, is_dirty);

    if (widget->as.box.direction == IuiBoxDirectionVertical)
      child_bounds.y += child_bounds.w + widget->as.box.spacing;
    else
      child_bounds.x += child_bounds.z + widget->as.box.spacing;
  }
}

void iui_widgets_recompute_layout(IuiWidgets *widgets, Vec4 bounds) {
  iui_widget_recompute_layout(widgets->root_widget, bounds,
                              &widgets->is_dirty);
}

void iui_widgets_abs_bounds(IuiWidgets *widgets, Vec4 bounds) {
  widgets->use_abs_bounds = true;
  widgets->abs_bounds = bounds;
}

static bool iui_widget_id_eq(IuiWidgetId *a, IuiWidgetId *b) {
  return a->kind == b->kind &&
         a->depth == b->depth &&
         a->child_index == b->child_index;
}

static IuiWidget *iui_widgets_get_widget(IuiWidgets *widgets,
                                         IuiWidgetKind kind) {
  IuiWidget *result = NULL;

  IuiWidgetId id;
  id.kind = kind;
  id.depth = widgets->boxes.len;
  id.child_index = 0;
  if (id.depth > 0)
    id.child_index = widgets->boxes.items[id.depth - 1]->children.len;

  IuiWidget *widget = widgets->list;
  while (widget) {
    if (iui_widget_id_eq(&widget->id, &id)) {
      result = widget;
      break;
    }

    widget = widget->next;
  }

  if (!result) {
    LL_PREPEND(widgets->list, widgets->list_end, IuiWidget);
    *widgets->list_end = (IuiWidget) {0};
    widgets->list_end->id = id;
    widgets->list_end->kind = kind;
    result = widgets->list_end;
  }

  if (widgets->boxes.len > 0) {
    IuiChildren *children = &widgets->boxes.items[widgets->boxes.len - 1]->children;
    DA_APPEND(*children, result);
  } else {
    widgets->root_widget = result;
  }

  if (widgets->use_abs_bounds) {
    widgets->use_abs_bounds = false;
    result->use_abs_bounds = true;
    result->bounds = widgets->abs_bounds;
  }

  return result;
}

IuiWidget *iui_widgets_push_box_begin(IuiWidgets *widgets, Vec2 margin,
                                      f32 spacing, IuiBoxDirection direction) {
  IuiWidget *widget = iui_widgets_get_widget(widgets, IuiWidgetKindBox);
  widget->as.box.margin = margin;
  widget->as.box.spacing = spacing;
  widget->as.box.direction = direction;
  widget->as.box.children = (IuiChildren) {0};

  DA_APPEND(widgets->boxes, &widget->as.box);

  return widget;
}

void iui_widgets_push_box_end(IuiWidgets *widgets) {
  if (widgets->boxes.len > 0)
    --widgets->boxes.len;
}

IuiWidget *iui_widgets_push_button(IuiWidgets *widgets, Str text,
                                   ValueFunc on_click) {
  IuiWidget *widget = iui_widgets_get_widget(widgets, IuiWidgetKindButton);
  widget->as.button.text = text;
  widget->as.button.on_click = on_click;

  return widget;
}

IuiWidget *iui_widgets_push_text(IuiWidgets *widgets, Str text, bool center) {
  IuiWidget *widget = iui_widgets_get_widget(widgets, IuiWidgetKindText);
  widget->as.text.text = text;
  widget->as.text.center = center;

  return widget;
}
