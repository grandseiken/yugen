#include "render_util.h"

#include <boost/functional/hash.hpp>

Colour::Colour(float r, float g, float b)
  : r(r), g(g), b(b), a(1.f)
{
}

Colour::Colour(float r, float g, float b, float a)
  : r(r), g(g), b(b), a(a)
{
}

BatchedTexture::BatchedTexture(const GlTexture& sprite,
                               y::int32 frame_width, y::int32 frame_height)
  : sprite(sprite)
  , frame_width(frame_width)
  , frame_height(frame_height)
{
}

BatchedSprite::BatchedSprite(float left, float top,
                             float frame_x, float frame_y)
  : left(left)
  , top(top)
  , frame_x(frame_x)
  , frame_y(frame_y)
{
}

bool BatchedTextureLess::operator()(const BatchedTexture& l,
                                    const BatchedTexture& r) const
{
  if (l.sprite.get_handle() < r.sprite.get_handle()) {
    return true;
  }
  if (l.sprite.get_handle() > r.sprite.get_handle()) {
    return false;
  }
  if (l.frame_width < r.frame_width) {
    return true;
  }
  if (l.frame_width > r.frame_width) {
    return false;
  }
  return l.frame_height < r.frame_height;
}

void RenderBatch::add_sprite(
    const GlTexture& sprite, y::int32 frame_width, y::int32 frame_height,
    y::int32 left, y::int32 top, y::int32 frame_x, y::int32 frame_y)
{
  BatchedTexture bt(sprite, frame_width, frame_height);
  BatchedSprite bs(left, top, frame_x, frame_y);
  _map[bt].push_back(bs);
}

const RenderBatch::BatchedTextureMap& RenderBatch::get_map() const
{
  return _map;
}

static const GLushort quad_data[] = {0, 1, 2, 3};

RenderUtil::RenderUtil(GlUtil& gl)
  : _gl(gl)
  , _quad(gl.make_buffer<GLushort, 1>(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
                                      quad_data, sizeof(quad_data)))
  , _native_width(0)
  , _native_height(0)
  , _font(gl.make_texture("/font.png"))
  , _text_program(gl.make_program({"/shaders/text.v.glsl",
                                   "/shaders/text.f.glsl"}))
  , _draw_program(gl.make_program({"/shaders/draw.v.glsl",
                                   "/shaders/draw.f.glsl"}))
  , _pixels_buffer(gl.make_buffer<float, 2>(GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _origin_buffer(gl.make_buffer<float, 2>(GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _frame_index_buffer(
      gl.make_buffer<float, 2>(GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _element_buffer(
      gl.make_buffer<GLushort, 1>(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW))
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
  _gl.delete_buffer(_pixels_buffer);
  _gl.delete_buffer(_origin_buffer);
  _gl.delete_buffer(_frame_index_buffer);
  _gl.delete_buffer(_element_buffer);
}

GlUtil& RenderUtil::get_gl() const
{
  return _gl;
}

const Window& RenderUtil::get_window() const
{
  return get_gl().get_window();
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
      float(left), float(top + font_height),
      float(left + _native_width), float(top + font_height)};

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
  _text_program.bind_uniform(
      "font_size", GLint(font_width), GLint(font_height));
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
  render_text(text, left * font_width, top * font_height, r, g, b, a);
}

void RenderUtil::render_text_grid(const y::string& text, int left, int top,
                                  const Colour& colour) const
{
  render_text_grid(text, left, top, colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::render_colour(y::int32 left, y::int32 top,
                               y::int32 width, y::int32 height,
                               float r, float g, float b, float a)
{
  if (!_native_width || !_native_height) {
    return;
  }

  const GLfloat rect_data[] = {
      float(left), float(top),
      float(left + width), float(top),
      float(left), float(top + height),
      float(left + width), float(top + height)};

  auto rect_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, rect_data, sizeof(rect_data));

  _draw_program.bind();
  _draw_program.bind_attribute("pixels", rect_buffer);
  _draw_program.bind_uniform("resolution",
      GLint(_native_width), GLint(_native_height));
  _draw_program.bind_uniform("colour", r, g, b, a);
  _quad.draw_elements(GL_TRIANGLE_STRIP, 4);

  _gl.delete_buffer(rect_buffer);
}

void RenderUtil::render_colour(y::int32 left, y::int32 top,
                               y::int32 width, y::int32 height,
                               const Colour& colour)
{
  render_colour(left, top, width, height,
                colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::render_colour_grid(y::int32 left, y::int32 top,
                                    y::int32 width, y::int32 height,
                                    float r, float g, float b, float a)
{
  render_colour(left * font_width, top * font_height,
                width * font_width, height * font_height, r, g, b, a);
}

void RenderUtil::render_colour_grid(y::int32 left, y::int32 top,
                                    y::int32 width, y::int32 height,
                                    const Colour& colour)
{
  render_colour_grid(left, top, width, height,
                     colour.r, colour.g, colour.b, colour.a);
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
void write_vector(std::vector<T>& dest, y::size dest_index,
                         const std::vector<T>& source)
{
  for (y::size i = 0; i < source.size(); ++i) {
    if (dest_index + i < dest.size()) {
      dest[dest_index + i] = source[i];
    }
    else {
      dest.push_back(source[i]);
    }
  }
}

void RenderUtil::render_batch() const
{
  if (!_native_width || !_native_height ||
      !_sprite || !_frame_width || !_frame_height) {
    return;
  }

  y::size length = _batched_sprites.size();
  for (y::size i = 0; i < length; ++i) {
    const BatchedSprite& s = _batched_sprites[i];

    write_vector(_pixels_data, 8 * i, {
        s.left, s.top,
        s.left + _frame_width, s.top,
        s.left, s.top + _frame_height,
        s.left + _frame_width, s.top + _frame_height});
    write_vector(_origin_data, 8 * i, {
        s.left, s.top, s.left, s.top,
        s.left, s.top, s.left, s.top});
    write_vector(_frame_index_data, 8 * i, {
        s.frame_x, s.frame_y, s.frame_x, s.frame_y,
        s.frame_x, s.frame_y, s.frame_x, s.frame_y});
  }

  _pixels_buffer.reupload_data(&_pixels_data[0], sizeof(float) * 8 * length);
  _origin_buffer.reupload_data(&_origin_data[0], sizeof(float) * 8 * length);
  _frame_index_buffer.reupload_data(&_frame_index_data[0],
                                    sizeof(float) * 8 * length);

  if (_element_data.size() / 6 < length) {
    for (y::size i = _element_data.size() / 6; i < length; ++i) {
      write_vector(_element_data, 6 * i, {
          GLushort(0 + 4 * i), GLushort(1 + 4 * i), GLushort(2 + 4 * i),
          GLushort(1 + 4 * i), GLushort(2 + 4 * i), GLushort(3 + 4 * i)});
    }
    _element_buffer.reupload_data(_element_data);
  }

  _sprite_program.bind();
  _sprite_program.bind_attribute("pixels", _pixels_buffer);
  _sprite_program.bind_attribute("origin", _origin_buffer);
  _sprite_program.bind_attribute("frame_index", _frame_index_buffer);
  _sprite_program.bind_uniform("resolution",
      GLint(_native_width), GLint(_native_height));

  _sprite.bind(GL_TEXTURE0);
  _sprite_program.bind_uniform("sprite", 0);
  _sprite_program.bind_uniform(
      "frame_size", GLint(_frame_width), GLint(_frame_height));
  _sprite_program.bind_uniform("frame_count",
                               GLint(_sprite.get_width() / _frame_width),
                               GLint(_sprite.get_height() / _frame_height));
  _element_buffer.draw_elements(GL_TRIANGLES, 6 * length);

  _batched_sprites.clear();
}

void RenderUtil::render_batch(const RenderBatch& batch)
{
  for (const auto& pair : batch.get_map()) {
    set_sprite(pair.first.sprite,
               pair.first.frame_width, pair.first.frame_height);
    _batched_sprites.insert(_batched_sprites.begin(),
                            pair.second.begin(), pair.second.end());
    render_batch();
  }
}
