#ifndef RENDER_UTIL_H
#define RENDER_UTIL_H

#include "common.h"
#include "gl_util.h"
#include "vector.h"

struct Colour {
  Colour(float r, float g, float b);
  Colour(float r, float g, float b, float a);

  float r;
  float g;
  float b;
  float a;
};

// TODO: z-indexing.
struct BatchedTexture {
  BatchedTexture(const GlTexture& sprite, const y::ivec2& frame_size);

  GlTexture sprite;
  y::ivec2 frame_size;
};

struct BatchedSprite {
  BatchedSprite(float left, float top, float frame_x, float frame_y,
                float depth, const Colour& colour);

  float left;
  float top;
  float frame_x;
  float frame_y;
  float depth;
  Colour colour;
};

struct BatchedTextureLess {
  bool operator()(const BatchedTexture& l, const BatchedTexture& r) const;
};

// Helper class to automatically batch renders using the same texture.
class RenderBatch {
public:

  void add_sprite(const GlTexture& sprite, const y::ivec2& frame_size,
                  const y::ivec2& origin, const y::ivec2& frame,
                  float depth, const Colour& colour);

  typedef y::vector<BatchedSprite> BatchedSpriteList;
  typedef y::ordered_map<BatchedTexture, BatchedSpriteList,
                         BatchedTextureLess> BatchedTextureMap;
  const BatchedTextureMap& get_map() const;

private:

  BatchedTextureMap _map;

};

class Window;

// TODO: remove _grid functions?
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
  void set_resolution(const y::ivec2& native_size);

  // Offset all render operations.
  void add_translation(const y::ivec2& translation);

  // Render text (at pixel coordinates).
  void render_text(const y::string& text, const y::ivec2& origin,
                   float r, float g, float b, float a) const;
  void render_text(const y::string& text, const y::ivec2& origin,
                   const Colour& colour) const;

  // Render text (at font-size grid coordinates).
  void render_text_grid(const y::string& text, const y::ivec2& origin,
                        float r, float g, float b, float a) const;
  void render_text_grid(const y::string& text, const y::ivec2& origin,
                        const Colour& colour) const;

  // Render colour (at pixel coordinates).
  void render_fill(const y::ivec2& origin, const y::ivec2& size,
                   float r, float g, float b, float a) const;
  void render_fill(const y::ivec2& origin, const y::ivec2& size,
                   const Colour& colour) const;

  // Render colour (at font-size grid coordinates).
  void render_fill_grid(const y::ivec2& origin, const y::ivec2& size,
                        float r, float g, float b, float a) const;
  void render_fill_grid(const y::ivec2& origin, const y::ivec2& size,
                        const Colour& colour) const;

  // Render outline (at pixel coordinates).
  void render_outline(const y::ivec2& origin, const y::ivec2& size,
                      float r, float g, float b, float a) const;
  void render_outline(const y::ivec2& origin, const y::ivec2& size,
                      const Colour& colour) const;

  // Render outline (at font-size grid coordinates).
  void render_outline_grid(const y::ivec2& origin, const y::ivec2& size,
                           float r, float g, float b, float a) const;
  void render_outline_grid(const y::ivec2& origin, const y::ivec2& size,
                           const Colour& colour) const;

  // Batch and render sprites (at pixel coordinates).
  void set_sprite(const GlTexture& sprite, const y::ivec2& frame_size);
  void batch_sprite(const y::ivec2& origin, const y::ivec2& frame,
                    float depth, const Colour& colour) const;
  void render_batch() const;

  // Render an entire set of batches at once. Sprite set with set_sprite
  // is not preserved.
  void render_batch(const RenderBatch& batch);

  // Render a sprite immediately.
  void render_sprite(const GlTexture& sprite, const y::ivec2& frame_size,
                     const y::ivec2& origin, const y::ivec2& frame,
                     float depth, const Colour& colour);

  // Helpers for grid coordinares.
  static y::ivec2 from_grid(const y::ivec2& grid = {1, 1});
  static y::ivec2 to_grid(const y::ivec2& pixels);

private:

  // Font width and height.
  static const y::int32 font_width = 8;
  static const y::int32 font_height = 8;
  static const y::ivec2 font_size;

  GlUtil& _gl;
  GlQuad _quad;

  y::ivec2 _native_size;
  y::ivec2 _translation;

  GlTexture _font;
  GlProgram _text_program;
  GlProgram _draw_program;

  mutable y::vector<float> _pixels_data;
  mutable y::vector<float> _origin_data;
  mutable y::vector<float> _frame_index_data;
  mutable y::vector<float> _depth_data;
  mutable y::vector<float> _colour_data;
  mutable y::vector<GLushort> _element_data;

  GlBuffer<float, 2> _pixels_buffer;
  GlBuffer<float, 2> _origin_buffer;
  GlBuffer<float, 2> _frame_index_buffer;
  GlBuffer<float, 1> _depth_buffer;
  GlBuffer<float, 4> _colour_buffer;
  GlBuffer<GLushort, 1> _element_buffer;

  GlTexture _sprite;
  y::ivec2 _frame_size;
  mutable y::vector<BatchedSprite> _batched_sprites;
  GlProgram _sprite_program;

};

#endif
