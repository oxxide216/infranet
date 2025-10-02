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
  char          *file_path;
  u32            line;
  IuiWidget     *next;
};

typedef struct {
  IuiWidget *root_widget;
  IuiWidget *list;
  IuiWidget *list_end;
  IuiBoxes   boxes;
  bool       is_dirty;
} IuiWidgets;

void iui_widgets_recompute_layout(IuiWidgets *widgets, Vec4 bounds);

IuiWidget *box_get_child(IuiBox *box, u32 i);

IuiWidget *iui_widgets_push_box_begin(IuiWidgets *widgets, Vec2 margin,
                                      IuiBoxDirection direction);
void iui_widgets_push_box_end(IuiWidgets *widgets);
IuiWidget *iui_widgets_push_button(IuiWidgets *widgets, Str text,
                                   ValueFunc on_click);

#endif // IUI_WIDGETS_H
