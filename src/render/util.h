#ifndef RENDER_UTIL_H
#define RENDER_UTIL_H

#include "gl_handle.h"
#include "../vec.h"
#include <map>

class GlUtil;

template<typename T, typename U = std::vector<T>>
void write_vector(std::vector<T>& dest, std::size_t dest_index, const U& source)
{
  std::size_t i = 0;
  for (const auto& u : source) {
    if (dest_index + i < dest.size()) {
      dest[dest_index + i] = T(u);
    }
    else {
      dest.emplace_back(T(u));
    }
    ++i;
  }
}

namespace colour {
  const y::fvec4 outline{.7f, .7f, .7f, 1.f};
  const y::fvec4 dark_outline{.3f, .3f, .3f, 1.f};
  const y::fvec4 panel{.2f, .2f, .2f, .7f};
  const y::fvec4 faint_panel{.1f, .1f, .1f, .5f};
  const y::fvec4 dark_panel{0.f, 0.f, 0.f, .7f};
  const y::fvec4 item{.6f, .6f, .6f, 1.f};
  const y::fvec4 hover{.9f, .9f, .9f, .5f};
  const y::fvec4 highlight{.9f, .9f, .9f, .2f};
  const y::fvec4 select{.9f, .9f, .9f, 1.f};
  const y::fvec4 white{1.f, 1.f, 1.f, 1.f};
  const y::fvec4 black{0.f, 0.f, 0.f, 1.f};
  const y::fvec4 dark{.5f, .5f, .5f, 1.f};
  const y::fvec4 transparent{1.f, 1.f, 1.f, .5f};
}

// Helper class to automatically batch renders using the same texture.
class RenderBatch {
public:

  struct batched_texture {
    GlTexture2D sprite;
    y::ivec2 frame_size;
    bool normal;
  };

  struct batched_sprite {
    float left;
    float top;
    float frame_x;
    float frame_y;
    float depth;
    float rotation;
    y::fvec4 colour;
  };

  void add_sprite(
      const GlTexture2D& sprite, const y::ivec2& frame_size, bool normal,
      const y::fvec2& origin, const y::ivec2& frame,
      float depth, float rotation, const y::fvec4& colour);
  void iadd_sprite(const GlTexture2D& sprite, const y::ivec2& frame_size,
                   const y::ivec2& origin, const y::ivec2& frame,
                   float depth, const y::fvec4& colour);

  struct batched_texture_order {
    bool operator()(const batched_texture& l, const batched_texture& r) const;
  };

  typedef std::vector<batched_sprite> batched_sprite_list;
  typedef std::map<batched_texture, batched_sprite_list,
                   batched_texture_order> batched_texture_map;
  const batched_texture_map& get_map() const;
  void clear();

private:

  batched_texture_map _map;

};

class Window;

class RenderUtil {
public:

  static const std::int32_t native_width = 640;
  static const std::int32_t native_height = 360;
  static const y::ivec2 native_size;
  static const y::ivec2 native_overflow_size;

  RenderUtil(GlUtil& gl);
  RenderUtil(const RenderUtil&) = delete;
  RenderUtil& operator=(const RenderUtil&) = delete;

  /***/ GlUtil& get_gl() const;
  const Window& get_window() const;

  typedef GlBuffer<GLushort, 1> gl_quad_element;
  typedef GlBuffer<float, 2> gl_quad_vertex;

  // Element array for arbitrarily many quads.
  const GlDatabuffer<GLushort, 1>& quad_element(std::size_t length) const;
  // Element array for a quad.
  const gl_quad_element& quad_element() const;
  // Vertex positions for a quad.
  const gl_quad_vertex& quad_vertex() const;

  // Native (target framebuffer) resolution must be set for utility methods
  // to behave correctly.
  void set_resolution(const y::ivec2& native_size);
  const y::ivec2& get_resolution() const;

  // Offset all render operations.
  void add_translation(const y::fvec2& translation);
  void iadd_translation(const y::ivec2& translation);
  const y::fvec2& get_translation() const;

  // Scale all render operations.
  void set_scale(float scale);
  float get_scale() const;

  // Clear colour to value.
  void clear(const y::fvec4& colour) const;

  // Render text (at pixel coordinates).
  void render_text(const std::string& text, const y::fvec2& origin,
                   const y::fvec4& colour) const;
  void irender_text(const std::string& text, const y::ivec2& origin,
                    const y::fvec4& colour) const;

  // Render colour (at pixel coordinates).
  void render_fill(const y::fvec2& origin, const y::fvec2& size,
                   const y::fvec4& colour) const;
  void render_fill(const y::fvec2& a, const y::fvec2& b, const y::fvec2& c,
                   const y::fvec4& colour) const;
  void irender_fill(const y::ivec2& origin, const y::ivec2& size,
                    const y::fvec4& colour) const;
  void irender_fill(const y::ivec2& a, const y::ivec2& b, const y::ivec2& c,
                    const y::fvec4& colour) const;

  // Render lines (at pixel coordinates).
  void render_line(const y::fvec2& a, const y::fvec2& b,
                   const y::fvec4& colour) const;
  struct line {
    y::fvec2 a;
    y::fvec2 b;
  };
  void render_lines(const std::vector<line>& lines,
                    const y::fvec4& colour) const;

  // Render outline (at pixel coordinates).
  void render_outline(const y::fvec2& origin, const y::fvec2& size,
                      const y::fvec4& colour) const;
  void irender_outline(const y::ivec2& origin, const y::ivec2& size,
                       const y::fvec4& colour) const;

  // Render an entire batch of sprites at once (at pixel coordinates).
  void render_batch(
      const GlTexture2D& sprite, const y::ivec2& frame_size, bool normal,
      const RenderBatch::batched_sprite_list& list) const;
  void render_batch(const RenderBatch& batch) const;

  // Render a sprite immediately.
  void render_sprite(
      const GlTexture2D& sprite, const y::ivec2& frame_size, bool normal,
      const y::fvec2& origin, const y::ivec2& frame,
      float depth, float rotation, const y::fvec4& colour) const;
  void irender_sprite(const GlTexture2D& sprite, const y::ivec2& frame_size,
                      const y::ivec2& origin, const y::ivec2& frame,
                      float depth, const y::fvec4& colour) const;

  // Helpers for font-size grid coordinates.
  static y::ivec2 from_grid(const y::ivec2& grid = {1, 1});
  static y::ivec2 to_grid(const y::ivec2& pixels);

  // Helpers for standard uniform variables.
  void bind_pixel_uniforms(const GlProgram& program) const;

private:

  // Font width and height.
  static const std::int32_t font_width = 8;
  static const std::int32_t font_height = 8;
  static const y::ivec2 font_size;

  GlUtil& _gl;
  GlUnique<gl_quad_element> _quad_element;
  GlUnique<gl_quad_vertex> _quad_vertex;

  y::ivec2 _native_size;
  y::fvec2 _translation;
  float _scale;

  GlUnique<GlTexture2D> _font;
  GlUnique<GlProgram> _text_program;
  GlUnique<GlProgram> _draw_program;
  GlUnique<GlProgram> _sprite_program;

  GlDatabuffer<float, 2> _pixels;
  GlDatabuffer<float, 1> _rotation;
  GlDatabuffer<float, 2> _origin;
  GlDatabuffer<float, 2> _frame_index;
  GlDatabuffer<float, 1> _depth;
  GlDatabuffer<float, 4> _colour;
  GlDatabuffer<GLushort, 1> _element;

};

#endif
