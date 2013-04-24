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
                               const y::ivec2& frame_size)
  : sprite(sprite)
  , frame_size(frame_size)
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
  if (l.frame_size[xx] < r.frame_size[xx]) {
    return true;
  }
  if (l.frame_size[xx] > r.frame_size[xx]) {
    return false;
  }
  return l.frame_size[yy] < r.frame_size[yy];
}

void RenderBatch::add_sprite(const GlTexture& sprite,
                             const y::ivec2& frame_size,
                             const y::ivec2& origin, const y::ivec2& frame)
{
  BatchedTexture bt(sprite, frame_size);
  BatchedSprite bs(origin[xx], origin[yy], frame[xx], frame[yy]);
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
  , _native_size{0, 0}
  , _translation{0, 0}
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
  , _frame_size{0, 0}
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

void RenderUtil::set_resolution(const y::ivec2& native_size)
{
  _native_size = native_size;
}

void RenderUtil::add_translation(const y::ivec2& translation)
{
  _translation += translation;
}

void RenderUtil::render_text(const y::string& text, const y::ivec2& origin,
                             float r, float g, float b, float a) const
{
  if (!_native_size[xx] || !_native_size[yy]) {
    return;
  }

  y::ivec2 font_size = from_grid();
  y::ivec2 v = origin + _translation;
  const GLfloat text_data[] = {
      float(v[xx]), float(v[yy]),
      float((v + _native_size)[xx]), float(v[yy]),
      float(v[xx]), float((v + font_size)[yy]),
      float((v + _native_size)[xx]), float((v + font_size)[yy])};

  auto text_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, text_data, sizeof(text_data));

  _text_program.bind();
  _text_program.bind_attribute("pixels", text_buffer);
  _text_program.bind_uniform("resolution",
      GLint(_native_size[xx]), GLint(_native_size[yy]));
  _text_program.bind_uniform("origin",
      GLint(text_data[0]), GLint(text_data[0]));
  for (y::size i = 0; i < 1024; ++i) {
    _text_program.bind_uniform(i, "string",
        GLint(i < text.length() ? text[i] : 0));
  }
  _font.bind(GL_TEXTURE0);
  _text_program.bind_uniform("font", 0);
  _text_program.bind_uniform(
      "font_size", GLint(font_size[xx]), GLint(font_size[yy]));
  _text_program.bind_uniform("colour", r, g, b, a);
  _quad.draw_elements(GL_TRIANGLE_STRIP, 4);

  _gl.delete_buffer(text_buffer);
}

void RenderUtil::render_text(const y::string& text, const y::ivec2& origin,
                             const Colour& colour) const
{
  render_text(text, origin, colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::render_text_grid(const y::string& text, const y::ivec2& origin,
                                  float r, float g, float b, float a) const
{
  render_text(text, from_grid(origin), r, g, b, a);
}

void RenderUtil::render_text_grid(const y::string& text, const y::ivec2& origin,
                                  const Colour& colour) const
{
  render_text_grid(text, origin, colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::render_fill(const y::ivec2& origin, const y::ivec2& size,
                             float r, float g, float b, float a) const
{
  if (!_native_size[xx] || !_native_size[xx]) {
    return;
  }

  y::ivec2 v = origin + _translation;
  const GLfloat rect_data[] = {
      float(v[xx]), float(v[yy]),
      float((v + size)[xx]), float(v[yy]),
      float(v[xx]), float((v + size)[yy]),
      float((v + size)[xx]), float((v + size)[yy])};

  auto rect_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, rect_data, sizeof(rect_data));

  _draw_program.bind();
  _draw_program.bind_attribute("pixels", rect_buffer);
  _draw_program.bind_uniform("resolution",
      GLint(_native_size[xx]), GLint(_native_size[yy]));
  _draw_program.bind_uniform("colour", r, g, b, a);
  _quad.draw_elements(GL_TRIANGLE_STRIP, 4);

  _gl.delete_buffer(rect_buffer);
}

void RenderUtil::render_fill(const y::ivec2& origin, const y::ivec2& size,
                             const Colour& colour) const
{
  render_fill(origin, size, colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::render_fill_grid(const y::ivec2& origin,
                                  const y::ivec2& size,
                                  float r, float g, float b, float a) const
{
  render_fill(from_grid(origin), from_grid(size), r, g, b, a);
}

void RenderUtil::render_fill_grid(const y::ivec2& origin,
                                  const y::ivec2& size,
                                  const Colour& colour) const
{
  render_fill_grid(origin, size, colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::render_outline(const y::ivec2& origin, const y::ivec2& size,
                                float r, float g, float b, float a) const
{
  render_fill(origin, {size[xx] - 1, 1}, r, g, b, a);
  render_fill(origin + y::ivec2{0, 1}, {1, size[yy] - 1}, r, g, b, a);
  render_fill(origin + y::ivec2{1, size[yy] - 1}, {size[xx] - 1, 1}, r, g, b, a);
  render_fill(origin + y::ivec2{size[xx] - 1, 0}, {1, size[yy] - 1}, r, g, b, a);
}

void RenderUtil::render_outline(const y::ivec2& origin, const y::ivec2& size,
                                const Colour& colour) const
{
  render_outline(origin, size, colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::render_outline_grid(
    const y::ivec2& origin, const y::ivec2& size,
    float r, float g, float b, float a) const
{
  render_outline(from_grid(origin), from_grid(size), r, g, b, a);
}

void RenderUtil::render_outline_grid(
    const y::ivec2& origin, const y::ivec2& size, const Colour& colour) const
{
  render_outline_grid(origin, size, colour.r, colour.g, colour.b, colour.a);
}

void RenderUtil::set_sprite(const GlTexture& texture,
                            const y::ivec2& frame_size)
{
  _sprite = texture;
  _frame_size = frame_size;
  _batched_sprites.clear();
}

void RenderUtil::batch_sprite(const y::ivec2& origin,
                              const y::ivec2& frame) const
{
  _batched_sprites.push_back(BatchedSprite(
      origin[xx], origin[yy], frame[xx], frame[yy]));
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
  if (!_native_size[xx] || !_native_size[yy] ||
      !_sprite || !_frame_size[xx] || !_frame_size[yy]) {
    return;
  }

  y::size length = _batched_sprites.size();
  for (y::size i = 0; i < length; ++i) {
    const BatchedSprite& s = _batched_sprites[i];

    float left = s.left + _translation[xx];
    float top = s.top + _translation[yy];

    write_vector(_pixels_data, 8 * i, {
        left, top,
        left + _frame_size[xx], top,
        left, top + _frame_size[yy],
        left + _frame_size[xx], top + _frame_size[yy]});
    write_vector(_origin_data, 8 * i, {
        left, top, left, top,
        left, top, left, top});
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
      GLint(_native_size[xx]), GLint(_native_size[yy]));

  y::ivec2 v = _sprite.get_size() / _frame_size;
  _sprite.bind(GL_TEXTURE0);
  _sprite_program.bind_uniform("sprite", 0);
  _sprite_program.bind_uniform(
      "frame_size", GLint(_frame_size[xx]), GLint(_frame_size[yy]));
  _sprite_program.bind_uniform("frame_count", GLint(v[xx]), GLint(v[yy]));
  _element_buffer.draw_elements(GL_TRIANGLES, 6 * length);

  _batched_sprites.clear();
}

void RenderUtil::render_batch(const RenderBatch& batch)
{
  for (const auto& pair : batch.get_map()) {
    set_sprite(pair.first.sprite, pair.first.frame_size);
    _batched_sprites.insert(_batched_sprites.begin(),
                            pair.second.begin(), pair.second.end());
    render_batch();
  }
}

void RenderUtil::render_sprite(
    const GlTexture& sprite, const y::ivec2& frame_size,
    const y::ivec2& origin, const y::ivec2& frame)
{
  set_sprite(sprite, frame_size);
  batch_sprite(origin, frame);
  render_batch();
}

y::ivec2 RenderUtil::from_grid(const y::ivec2& grid)
{
  return grid * font_size;
}

y::ivec2 RenderUtil::to_grid(const y::ivec2& pixels)
{
  return pixels / font_size;
}

const y::ivec2 RenderUtil::font_size{
    RenderUtil::font_width, RenderUtil::font_height};
