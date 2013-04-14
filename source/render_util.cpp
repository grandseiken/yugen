#include "render_util.h"

Colour::Colour(float r, float g, float b)
  : r(r), g(g), b(b), a(1.f)
{
}

Colour::Colour(float r, float g, float b, float a)
  : r(r), g(g), b(b), a(a)
{
}

static const GLushort quad_data[] = {0, 1, 2, 3};

RenderUtil::RenderUtil(GlUtil& gl)
  : _gl(gl)
  , _quad(gl.make_buffer<GLushort, 1>(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
                                      quad_data, sizeof(quad_data)))
  , _font(gl.make_texture("/font.png"))
  , _text_program(gl.make_program({"/shaders/text.v.glsl",
                                   "/shaders/text.f.glsl"}))
  , _native_width(0)
  , _native_height(0)
  , _sprite(gl.make_texture("/yedit/missing.png"))
  , _frame_width(0)
  , _frame_height(0)
  , _sprite_program(gl.make_program({"/shaders/sprite.v.glsl",
                                     "/shaders/sprite.f.glsl"}))
{
}

RenderUtil::~RenderUtil()
{
  _gl.delete_buffer(_quad);
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

  const GLfloat text_data[] = {
      float(left), float(top),
      float(left + _native_width), float(top),
      float(left), float(top + height),
      float(left + _native_width), float(top + height)};

  auto text_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, text_data, sizeof(text_data));

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

void RenderUtil::render_text(const y::string& text, int left, int top,
                             const Colour& colour) const
{
  render_text(text, left, top, colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::render_text_grid(const y::string& text, int left, int top,
                                  float r, float g, float b, float a) const
{
  render_text(text, left * width, top * height, r, g, b, a); 
}

void RenderUtil::render_text_grid(const y::string& text, int left, int top,
                                  const Colour& colour) const
{
  render_text_grid(text, left, top, colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::set_sprite(const GlTexture& texture,
                            int frame_width, int frame_height)
{
  _sprite = texture;
  _frame_width = frame_width;
  _frame_height = frame_height;
  _batched_sprites.clear();
}

void RenderUtil::batch_sprite(int left, int top,
                               int frame_x, int frame_y) const
{
  _batched_sprites.push_back(BatchedSprite(left, top, frame_x, frame_y));
}

template<typename T>
void write_subarray(T* array, const std::vector<T>& data)
{
  for (y::size i = 0; i < data.size(); ++i) {
    *(array + i) = data[i];
  }
}

void RenderUtil::render_batch() const
{
  if (!_native_width || !_native_height ||
      !_sprite || !_frame_width || !_frame_height) {
    return;
  }

  const y::size length = _batched_sprites.size();
  float* pixels_data = new float[8 * length];
  float* origin_data = new float[8 * length];
  float* frame_index_data = new float[8 * length];
  GLushort* element_data = new GLushort[6 * length];
  for (y::size i = 0; i < length; ++i) {
    const BatchedSprite& s = _batched_sprites[i];
    write_subarray(pixels_data + 8 * i, {
        s.left, s.top,
        s.left + _frame_width, s.top,
        s.left, s.top + _frame_height,
        s.left + _frame_width, s.top + _frame_height});
    write_subarray(element_data + 6 * i, {
        GLushort(0 + 4 * i), GLushort(1 + 4 * i), GLushort(2 + 4 * i),
        GLushort(1 + 4 * i), GLushort(2 + 4 * i), GLushort(3 + 4 * i)});
    for (y::size j = 0; j < 4; ++j) {
      write_subarray(origin_data + 2 * j + 8 * i, {s.left, s.top});
      write_subarray(frame_index_data + 2 * j + 8 * i, {s.frame_x, s.frame_y});
    }
  }

  auto pixels_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, pixels_data,
      sizeof(float) * 8 * length);
  auto origin_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, origin_data,
      sizeof(float) * 8 * length);
  auto frame_index_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, frame_index_data,
      sizeof(float) * 8 * length);
  auto element_buffer = _gl.make_buffer<GLushort, 1>(
      GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, element_data,
      sizeof(GLushort) * 6 * length);
  
  _sprite_program.bind();
  _sprite_program.bind_attribute("pixels", pixels_buffer);
  _sprite_program.bind_attribute("origin", origin_buffer);
  _sprite_program.bind_attribute("frame_index", frame_index_buffer);
  _sprite_program.bind_uniform("resolution",
      GLint(_native_width), GLint(_native_height));

  _sprite.bind(GL_TEXTURE0);
  _sprite_program.bind_uniform("sprite", 0);
  _sprite_program.bind_uniform(
      "frame_size", GLint(_frame_width), GLint(_frame_height));
  _sprite_program.bind_uniform("frame_count",
                               GLint(_sprite.get_width() / _frame_width),
                               GLint(_sprite.get_height() / _frame_height));
  element_buffer.draw_elements(GL_TRIANGLES, 6 * _batched_sprites.size());

  _gl.delete_buffer(pixels_buffer);
  _gl.delete_buffer(origin_buffer);
  _gl.delete_buffer(frame_index_buffer);
  _gl.delete_buffer(element_buffer);

  delete[] pixels_data;
  delete[] origin_data;
  delete[] frame_index_data;
  delete[] element_data;

  _batched_sprites.clear();
}

RenderUtil::BatchedSprite::BatchedSprite(float left, float top,
                                         float frame_x, float frame_y)
  : left(left)
  , top(top)
  , frame_x(frame_x)
  , frame_y(frame_y)
{
}
