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

struct BatchedTexture {
  BatchedTexture(const GlTexture& sprite,
                 y::int32 frame_width, y::int32 frame_height);

  GlTexture sprite;
  y::int32 frame_width;
  y::int32 frame_height;
};

struct BatchedSprite {
  BatchedSprite(float left, float top, float frame_x, float frame_y);

  float left;
  float top;
  float frame_x;
  float frame_y;
};

struct BatchedTextureLess {
  bool operator()(const BatchedTexture& l, const BatchedTexture& r) const;
};

// Helper class to automatically batch renders using the same texture.
class RenderBatch {
public:

  void add_sprite(const GlTexture& sprite,
                  y::int32 frame_width, y::int32 frame_height,
                  y::int32 left, y::int32 top,
                  y::int32 frame_x, y::int32 frame_y);

  typedef y::vector<BatchedSprite> BatchedSpriteList;
  typedef y::ordered_map<BatchedTexture, BatchedSpriteList,
                         BatchedTextureLess> BatchedTextureMap;
  const BatchedTextureMap& get_map() const;

private:

  BatchedTextureMap _map;

};

class Window;

class RenderUtil {
public:

  RenderUtil(GlUtil& gl);
  ~RenderUtil();

  /***/ GlUtil& get_gl() const;
  const Window& get_window() const;

  typedef GlBuffer<GLushort, 1> GlQuad;

  // Element array for a quad.
  const GlQuad& quad() const;

  // Native (target framebuffer) resolution must be set for utility methods
  // to behave correctly.
  void set_resolution(y::size native_width, y::size native_height);

  // Render text (at pixel coordinates).
  void render_text(const y::string& text, y::int32 left, y::int32 top,
                   float r, float g, float b, float a) const;
  void render_text(const y::string& text, y::int32 left, y::int32 top,
                   const Colour& colour) const;

  // Render text (at font-size grid coordinates).
  void render_text_grid(const y::string& text, y::int32 left, y::int32 top,
                        float r, float g, float b, float a) const;
  void render_text_grid(const y::string& text, y::int32 left, y::int32 top,
                        const Colour& colour) const;

  // Render colour (at pixel coordinates).
  void render_colour(y::int32 left, y::int32 top,
                     y::int32 width, y::int32 height,
                     float r, float g, float b, float a) const;
  void render_colour(y::int32 left, y::int32 top,
                     y::int32 width, y::int32 height,
                     const Colour& colour) const;

  // Render colour (at font-size grid coordinates).
  void render_colour_grid(y::int32 left, y::int32 top,
                          y::int32 width, y::int32 height,
                          float r, float g, float b, float a) const;
  void render_colour_grid(y::int32 left, y::int32 top,
                          y::int32 width, y::int32 height,
                          const Colour& colour) const;

  // Batch and render sprites (at pixel coordinates).
  void set_sprite(const GlTexture& sprite,
                  y::int32 frame_width, y::int32 frame_height);
  void batch_sprite(y::int32 left, y::int32 top,
                    y::int32 frame_x, y::int32 frame_y) const;
  void render_batch() const;

  // Render an entire set of batches at once. Sprite set with set_sprite
  // is not preserved.
  void render_batch(const RenderBatch& batch);

  // Render a sprite immediately.
  void render_sprite(const GlTexture& sprite,
                     y::int32 frame_width, y::int32 frame_height,
                     y::int32 left, y::int32 top,
                     y::int32 frame_x, y::int32 frame_y);

  // Font width and height.
  static const y::size font_width = 8;
  static const y::size font_height = 8;

private:

  GlUtil& _gl;
  GlQuad _quad;

  y::size _native_width;
  y::size _native_height;

  GlTexture _font;
  GlProgram _text_program;
  GlProgram _draw_program;

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
