#ifndef IUI_H
#define IUI_H

#include "../src/widgets.h"
#include "../src/renderer.h"
#include "aether-vm/vm.h"
#include "winx/winx.h"
#include "winx/event.h"
#include "glass/glass.h"
#include "glass/math.h"

typedef Da(WinxEvent) IuiEvents;

typedef struct {
  Winx        winx;
  WinxWindow  window;
  IuiEvents   events;
  IuiWidgets  widgets;
  IuiRenderer renderer;
} Iui;

void iui_push_intrinsics(Intrinsics *intrinsics);

#endif // IUI_H
