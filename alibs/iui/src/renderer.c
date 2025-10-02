#include "renderer.h"
#include "widgets.h"
#include "glass/glass.h"
#include "glass/params.h"

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

  return renderer;
}

void iui_renderer_resize(IuiRenderer *renderer, f32 width, f32 height) {
  glass_set_param_2f(&renderer->general_shader, "u_resolution",
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
  }

  glass_clear_screen(0.0, 0.0, 0.0, 0.0);
  glass_render_object(&renderer->general_object, NULL, 0);
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
