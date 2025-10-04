#include <stdlib.h>

#include "renderer.h"
#include "widgets.h"
#include "glass/glass.h"
#include "glass/params.h"
#include "../fonts/JetBrainsMono-Regular.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define TEXT_SIZE_MULTIPLIER    0.55
#define TEXT_QUALITY_MULTIPLIER 2.0

static Str general_vertex_shader_src = STR_LIT(
  "#version 330 core\n"
  "uniform vec2 u_resolution;\n"
  "layout (location = 0) in vec2 i_pos;\n"
  "layout (location = 1) in vec4 i_color;\n"
  "out vec4 o_color;\n"
  "void main(void) {\n"
  "  float x = i_pos.x / u_resolution.x * 2.0 - 1.0;\n"
  "  float y = 1.0 - i_pos.y / u_resolution.y * 2.0;\n"
  "  gl_Position = vec4(x, y, 0.0, 1.0);\n"
  "  o_color = i_color;\n"
  "}\n"
);

static Str general_fragment_shader_src = STR_LIT(
  "#version 330 core\n"
  "in vec4 o_color;\n"
  "out vec4 frag_color;\n"
  "void main(void) {\n"
  "  frag_color = o_color;\n"
  "}\n"
);

static Str texture_vertex_shader_src = STR_LIT(
  "#version 330 core\n"
  "uniform vec2 u_resolution;\n"
  "layout (location = 0) in vec2 i_pos;\n"
  "layout (location = 1) in vec2 i_uv;\n"
  "layout (location = 2) in vec4 i_color;\n"
  "out vec2 o_uv;\n"
  "out vec4 o_color;\n"
  "void main(void) {\n"
  "  float x = i_pos.x / u_resolution.x * 2.0 - 1.0;\n"
  "  float y = 1.0 - i_pos.y / u_resolution.y * 2.0;\n"
  "  gl_Position = vec4(x, y, 0.0, 1.0);\n"
  "  o_uv = i_uv;\n"
  "  o_color = i_color;\n"
  "}\n"
);

static Str texture_fragment_shader_src = STR_LIT(
  "#version 330 core\n"
  "uniform sampler2D u_texture;\n"
  "in vec2 o_uv;\n"
  "in vec4 o_color;\n"
  "out vec4 frag_color;\n"
  "void main(void) {\n"
  "  vec4 color = o_color;\n"
  "  color.a *= texture(u_texture, o_uv).r;\n"
  "  frag_color = color;\n"
  "}\n"
);

IuiRenderer iui_init_renderer(void) {
  IuiRenderer renderer = {0};

  glass_init();

  GlassAttributes general_attributes = {0};
  glass_push_attribute(&general_attributes, GlassAttributeKindFloat, 2);
  glass_push_attribute(&general_attributes, GlassAttributeKindFloat, 4);

  renderer.general_shader = glass_init_shader(general_vertex_shader_src,
                                              general_fragment_shader_src,
                                              &general_attributes);
  renderer.general_object = glass_init_object(&renderer.general_shader);

  GlassAttributes texture_attributes = {0};
  glass_push_attribute(&texture_attributes, GlassAttributeKindFloat, 2);
  glass_push_attribute(&texture_attributes, GlassAttributeKindFloat, 2);
  glass_push_attribute(&texture_attributes, GlassAttributeKindFloat, 4);

  renderer.texture_shader = glass_init_shader(texture_vertex_shader_src,
                                              texture_fragment_shader_src,
                                              &texture_attributes);
  renderer.texture_object = glass_init_object(&renderer.texture_shader);

  renderer.texture = glass_init_texture(GlassFilteringModeLinear);
  glass_set_param_1i(&renderer.texture_shader, "u_texture", 0);

  stbtt_InitFont(&renderer.default_font.info,
                 (u8 *) alibs_iui_fonts_JetBrainsMono_Regular_ttf,
                 0);

  stbtt_GetFontVMetrics(&renderer.default_font.info, &renderer.default_font.ascent,
                        &renderer.default_font.descent, &renderer.default_font.line_gap);

  return renderer;
}

void iui_renderer_resize(IuiRenderer *renderer, f32 width, f32 height) {
  glass_set_param_2f(&renderer->general_shader, "u_resolution",
                     vec2(width, height));
  glass_set_param_2f(&renderer->texture_shader, "u_resolution",
                     vec2(width, height));
}

static void iui_renderer_render_widget(IuiRenderer *renderer, IuiWidget *widget) {
  switch (widget->kind) {
  case IuiWidgetKindBox: {
    iui_renderer_push_quad(renderer, widget->bounds, vec4(0.05, 0.05, 0.05, 0.2));

    for (u32 i = 0; i < widget->as.box.children.len; ++i)
      iui_renderer_render_widget(renderer, widget->as.box.children.items[i]);
  } break;

  case IuiWidgetKindButton: {
    iui_renderer_push_quad(renderer, widget->bounds, vec4(1.0, 1.0, 1.0, 1.0));
    iui_renderer_push_text(renderer, widget->bounds, widget->as.button.text,
                           true, vec4(0.0, 0.0, 0.0, 1.0));
  } break;

  case IuiWidgetKindText: {
    iui_renderer_push_text(renderer, widget->bounds, widget->as.text.text,
                           widget->as.text.center, vec4(1.0, 1.0, 1.0, 1.0));
  } break;
  }
}

void iui_renderer_render_widgets(IuiRenderer *renderer, IuiWidgets *widgets) {
  if (widgets->is_dirty) {
    iui_renderer_render_widget(renderer, widgets->root_widget);

    glass_put_object_data(&renderer->general_object,
                          renderer->general_vertices.items,
                          renderer->general_vertices.len * sizeof(IuiGeneralVertex),
                          renderer->general_indices.items,
                          renderer->general_indices.len * sizeof(u32),
                          renderer->general_indices.len, true);

    renderer->general_vertices.len = 0;
    renderer->general_indices.len = 0;

    glass_put_object_data(&renderer->texture_object,
                          renderer->texture_vertices.items,
                          renderer->texture_vertices.len * sizeof(IuiTextureVertex),
                          renderer->texture_indices.items,
                          renderer->texture_indices.len * sizeof(u32),
                          renderer->texture_indices.len, true);

    renderer->texture_vertices.len = 0;
    renderer->texture_indices.len = 0;
  }

  glass_clear_screen(0.0, 0.0, 0.0, 0.0);
  glass_render_object(&renderer->general_object, NULL, 0);
  glass_render_object(&renderer->texture_object, &renderer->texture, 1);
}

void iui_renderer_push_quad(IuiRenderer *renderer, Vec4 bounds, Vec4 color) {
  DA_APPEND(renderer->general_indices, renderer->general_vertices.len);
  DA_APPEND(renderer->general_indices, renderer->general_vertices.len + 1);
  DA_APPEND(renderer->general_indices, renderer->general_vertices.len + 2);
  DA_APPEND(renderer->general_indices, renderer->general_vertices.len + 2);
  DA_APPEND(renderer->general_indices, renderer->general_vertices.len + 1);
  DA_APPEND(renderer->general_indices, renderer->general_vertices.len + 3);

  IuiGeneralVertex vertex0 = {
    vec2(bounds.x, bounds.y),
    color,
  };
  DA_APPEND(renderer->general_vertices, vertex0);

  IuiGeneralVertex vertex1 = {
    vec2(bounds.x + bounds.z, bounds.y),
    color,
  };
  DA_APPEND(renderer->general_vertices, vertex1);

  IuiGeneralVertex vertex2 = {
    vec2(bounds.x, bounds.y + bounds.w),
    color,
  };
  DA_APPEND(renderer->general_vertices, vertex2);

  IuiGeneralVertex vertex3 = {
    vec2(bounds.x + bounds.z, bounds.y + bounds.w),
    color,
  };
  DA_APPEND(renderer->general_vertices, vertex3);
}

void iui_renderer_push_texture(IuiRenderer *renderer, Vec4 bounds, Vec4 uv, Vec4 color) {
  DA_APPEND(renderer->texture_indices, renderer->texture_vertices.len);
  DA_APPEND(renderer->texture_indices, renderer->texture_vertices.len + 1);
  DA_APPEND(renderer->texture_indices, renderer->texture_vertices.len + 2);
  DA_APPEND(renderer->texture_indices, renderer->texture_vertices.len + 2);
  DA_APPEND(renderer->texture_indices, renderer->texture_vertices.len + 1);
  DA_APPEND(renderer->texture_indices, renderer->texture_vertices.len + 3);

  IuiTextureVertex vertex0 = {
    vec2(bounds.x, bounds.y),
    vec2(uv.x, uv.y),
    color,
  };
  DA_APPEND(renderer->texture_vertices, vertex0);

  IuiTextureVertex vertex1 = {
    vec2(bounds.x + bounds.z, bounds.y),
    vec2(uv.z, uv.y),
    color,
  };
  DA_APPEND(renderer->texture_vertices, vertex1);

  IuiTextureVertex vertex2 = {
    vec2(bounds.x, bounds.y + bounds.w),
    vec2(uv.x, uv.w),
    color,
  };
  DA_APPEND(renderer->texture_vertices, vertex2);

  IuiTextureVertex vertex3 = {
    vec2(bounds.x + bounds.z, bounds.y + bounds.w),
    vec2(uv.z, uv.w),
    color,
  };
  DA_APPEND(renderer->texture_vertices, vertex3);
}

static u8 *merge_texture_buffers(u8 *a, u32 a_rows, u32 a_cols,
                                 u8 *b, u32 b_rows, u32 b_cols) {
  u32 rows = a_rows;
  if (rows < b_rows)
    rows = b_rows;

  u32 cols = a_cols + b_cols;

  u8 *new_buffer = malloc(rows * cols);
  memset(new_buffer, 0, rows * cols);

  for (u32 y = 0; y < a_rows; ++y)
    for (u32 x = 0; x < a_cols; ++x)
      new_buffer[cols * y + x] = a[a_cols * y + x];

  for (u32 y = 0; y < b_rows; ++y)
    for (u32 x = 0; x < b_cols; ++x)
      new_buffer[cols * y + x + a_cols] = b[b_cols * y + x];

  return new_buffer;
}

static IuiGlyph *iui_renderer_get_glyph(IuiRenderer *renderer,
                                        f32 line_height, char _char) {
  f32 scale = stbtt_ScaleForPixelHeight(&renderer->default_font.info,
                                        line_height * TEXT_QUALITY_MULTIPLIER);

  for (u32 i = 0; i < renderer->glyphs.len; ++i) {
    IuiGlyph *glyph = renderer->glyphs.items + i;

    if (glyph->_char == _char && glyph->font_scale == scale)
      return glyph;
  }

  i32 advance;
  stbtt_GetCodepointHMetrics(&renderer->default_font.info, _char, &advance, NULL);

  i32 x0, y0, x1, y1;
  stbtt_GetCodepointBitmapBox(&renderer->default_font.info, _char,
                              scale, scale, &x0, &y0, &x1, &y1);

  Vec2 size = {
    advance * scale,
    (y1 - y0),
  };

  void *bitmap = malloc(size.x * size.y);
  stbtt_MakeCodepointBitmap(&renderer->default_font.info, bitmap,
                            size.x, size.y, size.x, scale, scale, _char);

  Vec4 buffer_bounds = {
    renderer->texture_buffer_width,
    0.0,
    renderer->texture_buffer_width + size.x,
    size.y,
  };

  void *new_buffer = merge_texture_buffers(renderer->texture_buffer,
                                           renderer->texture_buffer_height,
                                           renderer->texture_buffer_width,
                                           bitmap, size.y, size.x);

  free(renderer->texture_buffer);
  free(bitmap);

  renderer->texture_buffer = new_buffer;
  renderer->texture_buffer_width += size.x;
  if (renderer->texture_buffer_height < size.y)
    renderer->texture_buffer_height = size.y;

  size.x *= TEXT_SIZE_MULTIPLIER / TEXT_QUALITY_MULTIPLIER;
  size.y *= TEXT_SIZE_MULTIPLIER / TEXT_QUALITY_MULTIPLIER;
  f32 baseline = (renderer->default_font.ascent + renderer->default_font.descent) *
                 scale * TEXT_SIZE_MULTIPLIER / TEXT_QUALITY_MULTIPLIER;
  f32 bearing_y = -y0 * TEXT_SIZE_MULTIPLIER / TEXT_QUALITY_MULTIPLIER;

  IuiGlyph glyph =  { _char, scale, baseline, bearing_y, size, buffer_bounds };
  DA_APPEND(renderer->glyphs, glyph);

  glass_put_texture_data(&renderer->texture, renderer->texture_buffer,
                         renderer->texture_buffer_width,
                         renderer->texture_buffer_height,
                         GlassPixelKindSingleColor);

  return renderer->glyphs.items + renderer->glyphs.len - 1;
}

void iui_renderer_push_text(IuiRenderer *renderer, Vec4 bounds, Str text, bool center, Vec4 color) {
  f32 line_height = bounds.z;
  if (line_height > bounds.w)
    line_height = bounds.w;

  f32 x_offset = 0.0;
  if (center) {
    f32 width = 0.0;
    for (u32 i = 0; i < text.len; ++i)
      width += iui_renderer_get_glyph(renderer, line_height, text.ptr[i])->size.x;

    x_offset = (bounds.z - width) / 2.0;
  }

  for (u32 i = 0; i < text.len; ++i) {
    IuiGlyph *glyph = iui_renderer_get_glyph(renderer, line_height, text.ptr[i]);

    f32 y_offset = glyph->baseline - glyph->bearing_y;
    if (center)
      y_offset = (bounds.w + glyph->baseline) / 2.0 - glyph->bearing_y;

    Vec4 glyph_bounds = {
      bounds.x + x_offset,
      bounds.y + y_offset,
      glyph->size.x,
      glyph->size.y,
    };

    x_offset += glyph->size.x;

    Vec4 uv = {
      glyph->buffer_bounds.x / renderer->texture_buffer_width,
      glyph->buffer_bounds.y / renderer->texture_buffer_height,
      glyph->buffer_bounds.z / renderer->texture_buffer_width,
      glyph->buffer_bounds.w / renderer->texture_buffer_height,
    };

    iui_renderer_push_texture(renderer, glyph_bounds, uv, color);
  }
}
