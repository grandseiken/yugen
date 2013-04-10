#ifndef RENDER_UTIL_H
#define RENDER_UTIL_H

#include "common.h"
#include "gl_util.h"

class RenderUtil {
public:

  RenderUtil(GlUtil& gl);

  typedef GlBuffer<GLushort, 1> GlQuad;

  // Element array for a quad.
  const GlQuad& quad() const;

  // Native (target framebuffer) resolution must be set for utility methods
  // to behave correctly.
  void set_resolution(y::size native_width, y::size native_height);

  void render_text(const y::string& text, int left, int top,
                   float r, float g, float b, float a) const;

private:

  GlUtil& _gl;
  GlQuad _quad;
  GlTexture _font;
  GlProgram _text_program;

  y::size _native_width;
  y::size _native_height;

};

#endif
