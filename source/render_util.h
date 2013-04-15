#ifndef RENDER_UTIL_H
#define RENDER_UTIL_H

#include "common.h"
#include "gl_util.h"

struct Colour {
  Colour(float r, float g, float b);
  Colour(float r, float g, float b, float a);

  float r;
  float g;
  float b;
  float a;
};

class RenderUtil {
public:

  RenderUtil(GlUtil& gl);
  ~RenderUtil();

  typedef GlBuffer<GLushort, 1> GlQuad;

  // Element array for a quad.
  const GlQuad& quad() const;

  // Native (target framebuffer) resolution must be set for utility methods
  // to behave correctly.
  void set_resolution(y::size native_width, y::size native_height);

  // Render text (at pixel coordinates).
  void render_text(const y::string& text, int left, int top,
                   float r, float g, float b, float a) const;
  void render_text(const y::string& text, int left, int top,
                   const Colour& colour) const;

  // Render text (at font-size grid coordinates).
  void render_text_grid(const y::string& text, int left, int top,
                        float r, float g, float b, float a) const;
  void render_text_grid(const y::string& text, int left, int top,
                        const Colour& colour) const;

  // Batch and render sprites (at pixel coordinates).
  void set_sprite(const GlTexture& sprite, int frame_width, int frame_height);
  void batch_sprite(int left, int top, int frame_x, int frame_y) const;
  void render_batch() const;

private:

  // Font width and height.
  const y::size width = 8;
  const y::size height = 8;

  GlUtil& _gl;
  GlQuad _quad;

  y::size _native_width;
  y::size _native_height;

  GlTexture _font;
  GlProgram _text_program;

  struct BatchedSprite {
    BatchedSprite(float left, float top, float frame_x, float frame_y);

    float left;
    float top;
    float frame_x;
    float frame_y;
  };

  mutable y::vector<float> _pixels_data;
  mutable y::vector<float> _origin_data;
  mutable y::vector<float> _frame_index_data;
  mutable y::vector<GLushort> _element_data;

  GlBuffer<float, 2> _pixels_buffer;
  GlBuffer<float, 2> _origin_buffer;
  GlBuffer<float, 2> _frame_index_buffer;
  GlBuffer<GLushort, 1> _element_buffer;

  GlTexture _sprite;
  int _frame_width;
  int _frame_height;
  mutable y::vector<BatchedSprite> _batched_sprites;
  GlProgram _sprite_program;

};

#endif
