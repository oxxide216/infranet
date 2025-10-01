#ifndef IUI_WIDGETS_H
#define IUI_WIDGETS_H

#include "aether-vm/vm.h"
#include "glass/math.h"

#define iui_widgets_push_box_begin(widgets, margin, direction)                  \
  iui_widgets_push_box_begin_id(widgets, margin, direction, __FILE__, __LINE__)

#define iui_widgets_push_button(widgets, text, on_click)                  \
  iui_widgets_push_button_id(widgets, text, on_click, __FILE__, __LINE__)

typedef struct IuiWidget IuiWidget;

typedef Da(IuiWidget *) IuiChildren;

typedef enum {
  IuiWidgetKindBox = 0,
  IuiWidgetKindButton,
} IuiWidgetKind;

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
void iui_widgets_reset_layout(IuiWidgets *widgets);

IuiWidget *box_get_child(IuiBox *box, u32 i);

IuiWidget *iui_widgets_push_box_begin_id(IuiWidgets *widgets, Vec2 margin,
                                         IuiBoxDirection direction,
                                         char *file_path, u32 line);
void iui_widgets_push_box_end(IuiWidgets *widgets);
IuiWidget *iui_widgets_push_button_id(IuiWidgets *widgets, Str text,
                                      ValueFunc on_click,
                                      char *file_path, u32 line);

#endif // IUI_WIDGETS_H
