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

private:

  const y::size width = 8;
  const y::size height = 8;

  GlUtil& _gl;
  GlQuad _quad;
  GlTexture _font;
  GlProgram _text_program;

  y::size _native_width;
  y::size _native_height;

};

#endif
