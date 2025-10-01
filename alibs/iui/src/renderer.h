#ifndef IUI_RENDERER_H
#define IUI_RENDERER_H

#include "widgets.h"
#include "glass/glass.h"
#include "glass/math.h"
#include "shl_defs.h"

typedef struct {
  Vec2 pos;
  Vec4 color;
} IuiGeneralVertex;

typedef Da(IuiGeneralVertex) IuiGeneralVertices;

typedef Da(u32) IuiIndices;

typedef struct {
  IuiGeneralVertices general_vertices;
  IuiIndices         general_indices;
  GlassShader        general_shader;
  GlassObject        general_object;
} IuiRenderer;

IuiRenderer iui_init_renderer(void);
void        iui_renderer_resize(IuiRenderer *renderer, f32 width, f32 height);
void        iui_renderer_render_widgets(IuiRenderer *renderer, IuiWidgets *widgets);

void iui_renderer_push_quad(IuiRenderer *renderer, Vec4 bounds, Vec4 color);

#endif // IUI_RENDERER_H
