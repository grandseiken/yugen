#include "render_util.h"

static const GLushort quad_data[] = {0, 1, 2, 3};

RenderUtil::RenderUtil(GlUtil& gl)
  : _gl(gl)
  , _quad(gl.make_buffer<GLushort, 1>(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
                                      quad_data, sizeof(quad_data)))
  , _font(gl.make_texture("/font.png"))
  , _text_program(gl.make_program({"/shaders/pixels.v.glsl",
                                   "/shaders/text.f.glsl"}))
  , _native_width(0)
  , _native_height(0)
{
}

const RenderUtil::GlQuad& RenderUtil::quad() const
{
  return _quad;
}

void RenderUtil::set_resolution(y::size native_width, y::size native_height)
{
  _native_width = native_width;
  _native_height = native_height;
}

void RenderUtil::render_text(const y::string& text, int left, int top,
                             float r, float g, float b, float a) const
{
  if (!_native_width || !_native_height) {
    return;
  }

  const y::size width = 8;
  const y::size height = 8;

  const GLfloat text_data[] = {
        float(left),                 float(top),
        float(left + _native_width), float(top),
        float(left),                 float(top + height),
        float(left + _native_width), float(top + height)};

  auto text_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, text_data, sizeof(text_data));

  _text_program.bind();
  _text_program.bind_attribute("pixels", text_buffer);
  _text_program.bind_uniform("resolution",
      GLint(_native_width), GLint(_native_height));
  _text_program.bind_uniform("origin",
      GLint(text_data[0]), GLint(text_data[0]));
  for (y::size i = 0; i < 1024; ++i) {
    _text_program.bind_uniform(i, "string",
        GLint(i < text.length() ? text[i] : 0));
  }
  _font.bind(GL_TEXTURE0);
  _text_program.bind_uniform("font", 0);
  _text_program.bind_uniform("font_size", GLint(width), GLint(height));
  _text_program.bind_uniform("colour", r, g, b, a);
  _quad.draw_elements(GL_TRIANGLE_STRIP, 4);

  _gl.delete_buffer(text_buffer);
}
