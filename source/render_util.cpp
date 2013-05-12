#include "render_util.h"

#include <boost/functional/hash.hpp>

void RenderBatch::add_sprite(const GlTexture& sprite,
                             const y::ivec2& frame_size,
                             const y::ivec2& origin, const y::ivec2& frame,
                             float depth, const y::fvec4& colour)
{
  batched_texture bt{sprite, frame_size};
  batched_sprite bs{float(origin[xx]), float(origin[yy]),
                    float(frame[xx]), float(frame[yy]), depth, colour};
  _map[bt].push_back(bs);
}

bool RenderBatch::batched_texture_order::operator()(
    const batched_texture& l, const batched_texture& r) const
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

const RenderBatch::batched_texture_map& RenderBatch::get_map() const
{
  return _map;
}

void RenderBatch::clear()
{
  _map.clear();
}

static const GLushort quad_data[] = {0, 1, 2, 3};

const y::ivec2 RenderUtil::native_size{RenderUtil::native_width,
                                       RenderUtil::native_height};

RenderUtil::RenderUtil(GlUtil& gl)
  : _gl(gl)
  , _quad(gl.make_buffer<GLushort, 1>(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
                                      quad_data, sizeof(quad_data)))
  , _native_size{0, 0}
  , _translation{0, 0}
  , _scale(1.f)
  , _font(gl.make_texture("/font.png"))
  , _text_program(gl.make_program({"/shaders/text.v.glsl",
                                   "/shaders/text.f.glsl"}))
  , _draw_program(gl.make_program({"/shaders/draw.v.glsl",
                                   "/shaders/draw.f.glsl"}))
  , _pixels_buffer(gl.make_buffer<float, 2>(GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _origin_buffer(gl.make_buffer<float, 2>(GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _frame_index_buffer(
      gl.make_buffer<float, 2>(GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _depth_buffer(gl.make_buffer<float, 1>(GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _colour_buffer(gl.make_buffer<float, 4>(GL_ARRAY_BUFFER, GL_STREAM_DRAW))
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
  _gl.delete_buffer(_frame_index_buffer);
  _gl.delete_buffer(_depth_buffer);
  _gl.delete_buffer(_colour_buffer);
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

const RenderUtil::gl_quad& RenderUtil::quad() const
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

void RenderUtil::set_scale(float scale)
{
  _scale = scale;
}

void RenderUtil::render_text(const y::string& text, const y::ivec2& origin,
                             const y::fvec4& colour) const
{
  if (!(_native_size >= y::ivec2())) {
    return;
  }
  _gl.enable_depth(false);

  y::ivec2 font_size = from_grid();
  const GLfloat text_data[] = {
      float(origin[xx]), float(origin[yy]),
      float((origin + _native_size)[xx]), float(origin[yy]),
      float(origin[xx]), float((origin + font_size)[yy]),
      float((origin + _native_size)[xx]), float((origin + font_size)[yy])};

  auto text_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, text_data, sizeof(text_data));

  _text_program.bind();
  _text_program.bind_attribute("pixels", text_buffer);
  _text_program.bind_uniform("resolution", _native_size);
  _text_program.bind_uniform("origin",
      GLint(text_data[0]), GLint(text_data[1]));
  for (y::size i = 0; i < 1024; ++i) {
    _text_program.bind_uniform(i, "string",
        GLint(i < text.length() ? text[i] : 0));
  }
  _font.bind(GL_TEXTURE0);
  _text_program.bind_uniform("font", 0);
  _text_program.bind_uniform("font_size", font_size);
  _text_program.bind_uniform("translation", y::fvec2(_translation));
  _text_program.bind_uniform("scale", y::fvec2{_scale, _scale});
  _text_program.bind_uniform("colour", colour);
  _quad.draw_elements(GL_TRIANGLE_STRIP, 4);

  _gl.delete_buffer(text_buffer);
}

void RenderUtil::render_fill(const y::ivec2& origin, const y::ivec2& size,
                             const y::fvec4& colour) const
{
  render_fill_internal(y::fvec2(origin), y::fvec2(size), colour);
}

void RenderUtil::render_outline(const y::ivec2& origin, const y::ivec2& size,
                                const y::fvec4& colour) const
{
  // In order to keep lines 1 pixel wide we need to divide by the scale.
  float pixel = 1.f / _scale;
  render_fill_internal(
      y::fvec2(origin),
      {size[xx] - pixel, pixel},
      colour);
  render_fill_internal(
      y::fvec2(origin) + y::fvec2{0.f, pixel},
      {pixel, size[yy] - pixel},
      colour);
  render_fill_internal(
      y::fvec2(origin) + y::fvec2{pixel, size[yy] - pixel},
      {size[xx] - pixel, pixel},
      colour);
  render_fill_internal(
      y::fvec2(origin) + y::fvec2{size[xx] - pixel, 0.f},
      {pixel, size[yy] - pixel},
      colour);
}

void RenderUtil::set_sprite(const GlTexture& texture,
                            const y::ivec2& frame_size)
{
  _sprite = texture;
  _frame_size = frame_size;
  _batched_sprites.clear();
}

void RenderUtil::batch_sprite(const y::ivec2& origin, const y::ivec2& frame,
                              float depth, const y::fvec4& colour) const
{
  _batched_sprites.push_back(RenderBatch::batched_sprite{
      float(origin[xx]), float(origin[yy]),
      float(frame[xx]), float(frame[yy]), depth, colour});
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
  if (!(_native_size >= y::ivec2()) ||
      !(_frame_size >= y::ivec2()) || !_sprite) {
    return;
  }
  _gl.enable_depth(true);

  y::size length = _batched_sprites.size();
  for (y::size i = 0; i < length; ++i) {
    const RenderBatch::batched_sprite& s = _batched_sprites[i];
    float left = s.left;
    float top = s.top;

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
    write_vector(_depth_data, 4 * i, {
        s.depth, s.depth, s.depth, s.depth});
    write_vector(_colour_data, 16 * i, {
        s.colour[rr], s.colour[gg], s.colour[bb], s.colour[aa],
        s.colour[rr], s.colour[gg], s.colour[bb], s.colour[aa],
        s.colour[rr], s.colour[gg], s.colour[bb], s.colour[aa],
        s.colour[rr], s.colour[gg], s.colour[bb], s.colour[aa]});
  }

  _pixels_buffer.reupload_data(&_pixels_data[0], sizeof(float) * 8 * length);
  _frame_index_buffer.reupload_data(&_frame_index_data[0],
                                    sizeof(float) * 8 * length);
  _origin_buffer.reupload_data(&_origin_data[0], sizeof(float) * 8 * length);
  _depth_buffer.reupload_data(&_depth_data[0], sizeof(float) * 4 * length);
  _colour_buffer.reupload_data(&_colour_data[0], sizeof(float) * 16 * length);

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
  _sprite_program.bind_attribute("depth", _depth_buffer);
  _sprite_program.bind_attribute("colour", _colour_buffer);
  _sprite_program.bind_uniform("resolution", _native_size);
  _sprite_program.bind_uniform("translation", y::fvec2(_translation));
  _sprite_program.bind_uniform("scale", y::fvec2{_scale, _scale});

  y::ivec2 v = _sprite.get_size() / _frame_size;
  _sprite.bind(GL_TEXTURE0);
  _sprite_program.bind_uniform("sprite", 0);
  _sprite_program.bind_uniform("frame_size", _frame_size);
  _sprite_program.bind_uniform("frame_count", v);
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
    const y::ivec2& origin, const y::ivec2& frame,
    float depth, const y::fvec4& colour)
{
  set_sprite(sprite, frame_size);
  batch_sprite(origin, frame, depth, colour);
  render_batch();
}

void RenderUtil::render_fill_internal(
    const y::fvec2& origin, const y::fvec2& size,
    const y::fvec4& colour) const
{
  if (!(_native_size >= y::ivec2())) {
    return;
  }
  _gl.enable_depth(false);

  const GLfloat rect_data[] = {
      origin[xx], origin[yy],
      (origin + size)[xx], origin[yy],
      origin[xx], (origin + size)[yy],
      (origin + size)[xx], (origin + size)[yy]};

  auto rect_buffer = _gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, rect_data, sizeof(rect_data));

  _draw_program.bind();
  _draw_program.bind_attribute("pixels", rect_buffer);
  _draw_program.bind_uniform("resolution", _native_size);
  _draw_program.bind_uniform("translation", y::fvec2(_translation));
  _draw_program.bind_uniform("scale", y::fvec2{_scale, _scale});
  _draw_program.bind_uniform("colour", colour);
  _quad.draw_elements(GL_TRIANGLE_STRIP, 4);

  _gl.delete_buffer(rect_buffer);
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
