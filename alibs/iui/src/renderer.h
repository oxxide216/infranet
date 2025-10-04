#ifndef IUI_RENDERER_H
#define IUI_RENDERER_H

#include "widgets.h"
#include "glass/glass.h"
#include "glass/math.h"
#include "shl_defs.h"
#include "stb_truetype.h"

typedef struct {
  Vec2 pos;
  Vec4 color;
} IuiGeneralVertex;

typedef Da(IuiGeneralVertex) IuiGeneralVertices;

typedef struct {
  Vec2 pos;
  Vec2 uv;
  Vec4 color;
} IuiTextureVertex;

typedef Da(IuiTextureVertex) IuiTextureVertices;

typedef Da(u32) IuiIndices;

typedef struct {
  char _char;
  f32  font_scale;
  f32  baseline;
  f32  bearing_y;
  Vec2 size;
  Vec4 buffer_bounds;
} IuiGlyph;

typedef Da(IuiGlyph) IuiGlyphs;

typedef struct {
  stbtt_fontinfo info;
  i32            ascent;
  i32            descent;
  i32            line_gap;
} IuiFont;

typedef struct {
  IuiGeneralVertices general_vertices;
  IuiIndices         general_indices;
  GlassShader        general_shader;
  GlassObject        general_object;
  IuiTextureVertices texture_vertices;
  IuiIndices         texture_indices;
  GlassShader        texture_shader;
  GlassObject        texture_object;
  GlassTexture       texture;
  void              *texture_buffer;
  u32                texture_buffer_width;
  u32                texture_buffer_height;
  IuiGlyphs          glyphs;
  IuiFont            default_font;
} IuiRenderer;

IuiRenderer iui_init_renderer(void);
void        iui_renderer_load_font(u8 *data);
void        iui_renderer_resize(IuiRenderer *renderer, f32 width, f32 height);
void        iui_renderer_render_widgets(IuiRenderer *renderer, IuiWidgets *widgets);

void iui_renderer_push_quad(IuiRenderer *renderer, Vec4 bounds, Vec4 color);
void iui_renderer_push_texture(IuiRenderer *renderer, Vec4 bounds, Vec4 uv, Vec4 color);
void iui_renderer_push_text(IuiRenderer *renderer, Vec4 bounds, Str text, bool center, Vec4 color);

#endif // IUI_RENDERER_H
