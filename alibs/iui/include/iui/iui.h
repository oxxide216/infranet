#ifndef IUI_H
#define IUI_H

#include "aether-vm/vm.h"
#include "winx/winx.h"

typedef struct {
  Winx       winx;
  WinxWindow window;
} Iui;

void iui_init(Str window_title,
              u32 window_width,
              u32 window_height);
void iui_push_intrinsics(Intrinsics *intrinsics);

#endif // IUI_H
