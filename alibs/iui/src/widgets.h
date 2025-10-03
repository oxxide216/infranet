#ifndef IUI_WIDGETS_H
#define IUI_WIDGETS_H

#include "aether-vm/vm.h"
#include "glass/math.h"

typedef struct IuiWidget IuiWidget;

typedef Da(IuiWidget *) IuiChildren;

typedef enum {
  IuiWidgetKindBox = 0,
  IuiWidgetKindButton,
} IuiWidgetKind;

typedef struct {
  IuiWidgetKind kind;
  u32           depth;
  u32           child_index;
} IuiWidgetId;

typedef enum {
  IuiBoxDirectionVertical = 0,
  IuiBoxDirectionHorizontal,
} IuiBoxDirection;

typedef struct {
  Vec2            margin;
  f32             spacing;
  IuiBoxDirection direction;
  IuiChildren     children;
} IuiBox;

typedef Da(IuiBox *) IuiBoxes;

typedef struct {
  Str text;
  ValueFunc on_click;
} IuiButton;

typedef union {
  IuiBox    box;
  IuiButton button;
} IuiWidgetAs;

struct IuiWidget {
  IuiWidgetId    id;
  IuiWidgetKind  kind;
  IuiWidgetAs    as;
  Vec4           bounds;
  bool           use_abs_bounds;
  IuiWidget     *next;
};

typedef struct {
  IuiWidget *root_widget;
  IuiWidget *list;
  IuiWidget *list_end;
  IuiBoxes   boxes;
  Vec4       abs_bounds;
  bool       use_abs_bounds;
  bool       is_dirty;
} IuiWidgets;

void iui_widgets_recompute_layout(IuiWidgets *widgets, Vec4 bounds);
void iui_widgets_abs_bounds(IuiWidgets *widgets, Vec4 bounds);

IuiWidget *iui_widgets_push_box_begin(IuiWidgets *widgets, Vec2 margin,
                                      f32 spacing, IuiBoxDirection direction);
void iui_widgets_push_box_end(IuiWidgets *widgets);
IuiWidget *iui_widgets_push_button(IuiWidgets *widgets, Str text,
                                   ValueFunc on_click);

#endif // IUI_WIDGETS_H
