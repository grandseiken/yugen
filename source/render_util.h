#ifndef RENDER_UTIL_H
#define RENDER_UTIL_H

#include "common.h"
#include "gl_handle.h"
#include "vector.h"

class GlUtil;

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
class RenderBatch : public y::no_copy {
public:

  struct batched_texture {
    GlTexture2D sprite;
    y::ivec2 frame_size;
  };

  struct batched_sprite {
    float left;
    float top;
    float frame_x;
    float frame_y;
    float depth;
    float rotation;
    // TODO: get rid of colour for sprites. Need to handle transparency
    // way different for lighting anyway, with explicit layers and everything.
    y::fvec4 colour;
  };

  void add_sprite(const GlTexture2D& sprite, const y::ivec2& frame_size,
                  const y::fvec2& origin, const y::ivec2& frame,
                  float depth, float rotation, const y::fvec4& colour);
  void iadd_sprite(const GlTexture2D& sprite, const y::ivec2& frame_size,
                   const y::ivec2& origin, const y::ivec2& frame,
                   float depth, const y::fvec4& colour);

  struct batched_texture_order {
    bool operator()(const batched_texture& l, const batched_texture& r) const;
  };

  typedef y::vector<batched_sprite> batched_sprite_list;
  typedef y::ordered_map<batched_texture, batched_sprite_list,
                         batched_texture_order> batched_texture_map;
  const batched_texture_map& get_map() const;
  void clear();

private:

  batched_texture_map _map;

};

class Window;

class RenderUtil : public y::no_copy {
public:

  static const y::int32 native_width = 640;
  static const y::int32 native_height = 360;
  static const y::ivec2 native_size;
  static const y::ivec2 native_overflow_size;

  RenderUtil(GlUtil& gl);
  ~RenderUtil();

  /***/ GlUtil& get_gl() const;
  const Window& get_window() const;

  typedef GlBuffer<GLushort, 1> gl_quad_element;
  typedef GlBuffer<float, 2> gl_quad_vertex;

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
  void render_text(const y::string& text, const y::fvec2& origin,
                   const y::fvec4& colour) const;
  void irender_text(const y::string& text, const y::ivec2& origin,
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
  void render_lines(const y::vector<line>& lines,
                    const y::fvec4& colour) const;

  // Render outline (at pixel coordinates).
  void render_outline(const y::fvec2& origin, const y::fvec2& size,
                      const y::fvec4& colour) const;
  void irender_outline(const y::ivec2& origin, const y::ivec2& size,
                       const y::fvec4& colour) const;

  // Batch and render sprites (at pixel coordinates).
  void set_sprite(const GlTexture2D& sprite, const y::ivec2& frame_size);
  void batch_sprite(const y::fvec2& origin, const y::ivec2& frame,
                    float depth, float rotation, const y::fvec4& colour) const;
  void render_batch() const;

  // Render an entire set of batches at once. Sprite set with set_sprite
  // is not preserved.
  void render_batch(const RenderBatch& batch);

  // Render a sprite immediately.
  void render_sprite(const GlTexture2D& sprite, const y::ivec2& frame_size,
                     const y::fvec2& origin, const y::ivec2& frame,
                     float depth, float rotation, const y::fvec4& colour);
  void irender_sprite(const GlTexture2D& sprite, const y::ivec2& frame_size,
                      const y::ivec2& origin, const y::ivec2& frame,
                      float depth, const y::fvec4& colour);

  // Helpers for font-size grid coordinates.
  static y::ivec2 from_grid(const y::ivec2& grid = {1, 1});
  static y::ivec2 to_grid(const y::ivec2& pixels);

  // Helpers for standard uniform variables.
  void bind_pixel_uniforms(const GlProgram& program) const;

private:

  // Font width and height.
  static const y::int32 font_width = 8;
  static const y::int32 font_height = 8;
  static const y::ivec2 font_size;

  GlUtil& _gl;
  gl_quad_element _quad_element;
  gl_quad_vertex _quad_vertex;

  y::ivec2 _native_size;
  y::fvec2 _translation;
  float _scale;

  GlTexture2D _font;
  GlProgram _text_program;
  GlProgram _draw_program;

  // TODO: figure out if this is really worth it, and if so, come up with a
  // more easily-reusable way to do it (lighting.h too).
  mutable y::vector<float> _pixels_data;
  mutable y::vector<float> _rotation_data;
  mutable y::vector<float> _origin_data;
  mutable y::vector<float> _frame_index_data;
  mutable y::vector<float> _depth_data;
  mutable y::vector<float> _colour_data;
  mutable y::vector<GLushort> _element_data;

  GlBuffer<float, 2> _pixels_buffer;
  GlBuffer<float, 1> _rotation_buffer;
  GlBuffer<float, 2> _origin_buffer;
  GlBuffer<float, 2> _frame_index_buffer;
  GlBuffer<float, 1> _depth_buffer;
  GlBuffer<float, 4> _colour_buffer;
  GlBuffer<GLushort, 1> _element_buffer;

  GlTexture2D _sprite;
  y::ivec2 _frame_size;
  mutable y::vector<RenderBatch::batched_sprite> _batched_sprites;
  GlProgram _sprite_program;

};

#endif
