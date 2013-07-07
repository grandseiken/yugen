#include "render_util.h"
#include "gl_util.h"

#include <boost/functional/hash.hpp>

void RenderBatch::add_sprite(
    const GlTexture2D& sprite, const y::ivec2& frame_size,
    const y::fvec2& origin, const y::ivec2& frame,
    float depth, float rotation,
    const y::fvec4& colour)
{
  batched_texture bt{sprite, frame_size};
  batched_sprite bs{origin[xx], origin[yy],
                    float(frame[xx]), float(frame[yy]),
                    depth, rotation, colour};
  _map[bt].push_back(bs);
}

void RenderBatch::iadd_sprite(
    const GlTexture2D& sprite, const y::ivec2& frame_size,
    const y::ivec2& origin, const y::ivec2& frame,
    float depth, const y::fvec4& colour)
{
  add_sprite(sprite, frame_size, y::fvec2(origin), frame, depth, 0.f, colour);
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
  for (auto& pair : _map) {
    pair.second.clear();
  }
}

static const GLushort quad_element_data[] = {0, 1, 2, 3};
static const float quad_vertex_data[] = {
  -1.f, -1.f,
   1.f, -1.f,
  -1.f, +1.f,
   1.f, +1.f};

const y::ivec2 RenderUtil::native_size{RenderUtil::native_width,
                                       RenderUtil::native_height};
// The size of the buffer we draw in order to handle rotations and other
// special effects which might depend on pixels outside the view frame.
const y::int32 native_overflow_dimensions =
    64 + RenderUtil::native_size.length();
const y::ivec2 RenderUtil::native_overflow_size{native_overflow_dimensions,
                                                native_overflow_dimensions};

RenderUtil::RenderUtil(GlUtil& gl)
  : _gl(gl)
  , _quad_element(gl.make_unique_buffer<GLushort, 1>(
        GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
        quad_element_data, sizeof(quad_element_data)))
  , _quad_vertex(gl.make_unique_buffer<GLfloat, 2>(
        GL_ARRAY_BUFFER, GL_STATIC_DRAW,
        quad_vertex_data, sizeof(quad_vertex_data)))
  , _native_size{0, 0}
  , _translation{0.f, 0.f}
  , _scale(1.f)
  , _font(gl.make_unique_texture("/font.png"))
  , _text_program(gl.make_unique_program({
        "/shaders/text.v.glsl",
        "/shaders/text.f.glsl"}))
  , _draw_program(gl.make_unique_program({
        "/shaders/draw.v.glsl",
        "/shaders/draw.f.glsl"}))
  , _sprite_program(gl.make_unique_program({
        "/shaders/sprite.v.glsl",
        "/shaders/sprite.f.glsl"}))
  , _pixels_buffer(gl.make_unique_buffer<float, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _rotation_buffer(gl.make_unique_buffer<float, 1>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _origin_buffer(gl.make_unique_buffer<float, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _frame_index_buffer(gl.make_unique_buffer<float, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _depth_buffer(gl.make_unique_buffer<float, 1>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _colour_buffer(gl.make_unique_buffer<float, 4>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _element_buffer(gl.make_unique_buffer<GLushort, 1>(
        GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW))
{
}

GlUtil& RenderUtil::get_gl() const
{
  return _gl;
}

const Window& RenderUtil::get_window() const
{
  return get_gl().get_window();
}

const RenderUtil::gl_quad_element& RenderUtil::quad_element() const
{
  return *_quad_element;
}

const RenderUtil::gl_quad_vertex& RenderUtil::quad_vertex() const
{
  return *_quad_vertex;
}

void RenderUtil::set_resolution(const y::ivec2& native_size)
{
  _native_size = native_size;
}

const y::ivec2& RenderUtil::get_resolution() const
{
  return _native_size;
}

void RenderUtil::add_translation(const y::fvec2& translation)
{
  _translation += translation;
}

void RenderUtil::iadd_translation(const y::ivec2& translation)
{
  add_translation(y::fvec2(translation));
}

const y::fvec2& RenderUtil::get_translation() const
{
  return _translation;
}

void RenderUtil::set_scale(float scale)
{
  _scale = scale;
}

float RenderUtil::get_scale() const
{
  return _scale;
}

void RenderUtil::clear(const y::fvec4& colour) const
{
  glClearColor(colour[rr], colour[gg], colour[bb], colour[aa]);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.f, 0.f, 0.f, 0.f);
}

void RenderUtil::render_text(const y::string& text, const y::fvec2& origin,
                             const y::fvec4& colour) const
{
  if (!(_native_size >= y::ivec2())) {
    return;
  }
  _gl.enable_depth(false);
  _gl.enable_blend(true);

  y::fvec2 font_size = y::fvec2(from_grid());
  y::vector<float> text_data{
      origin[xx], origin[yy],
      (origin + y::fvec2(_native_size))[xx], origin[yy],
      origin[xx], (origin + y::fvec2(font_size))[yy],
      (origin + y::fvec2(_native_size))[xx],
      (origin + y::fvec2(font_size))[yy]};

  auto text_buffer = _gl.make_unique_buffer<float, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, text_data);

  _text_program->bind();
  _text_program->bind_attribute("pixels", *text_buffer);
  _text_program->bind_uniform("origin", origin);
  for (y::size i = 0; i < 1024; ++i) {
    _text_program->bind_uniform(i, "string",
        GLint(i < text.length() ? text[i] : 0));
  }
  _text_program->bind_uniform("font", *_font);
  _text_program->bind_uniform("font_size", font_size);
  _text_program->bind_uniform("colour", colour);
  bind_pixel_uniforms(*_text_program);
  _quad_element->draw_elements(GL_TRIANGLE_STRIP, 4);
}

void RenderUtil::irender_text(const y::string& text, const y::ivec2& origin,
                              const y::fvec4& colour) const
{
  render_text(text, y::fvec2(origin), colour);
}

void RenderUtil::render_fill(const y::fvec2& origin, const y::fvec2& size,
                             const y::fvec4& colour) const
{
  if (!(_native_size >= y::ivec2())) {
    return;
  }
  _gl.enable_depth(false);
  _gl.enable_blend(true);

  const GLfloat rect_data[] = {
      origin[xx], origin[yy],
      (origin + size)[xx], origin[yy],
      origin[xx], (origin + size)[yy],
      (origin + size)[xx], (origin + size)[yy]};

  auto rect_buffer = _gl.make_unique_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, rect_data, sizeof(rect_data));

  _draw_program->bind();
  _draw_program->bind_attribute("pixels", *rect_buffer);
  _draw_program->bind_uniform("colour", colour);
  bind_pixel_uniforms(*_draw_program);
  _quad_element->draw_elements(GL_TRIANGLE_STRIP, 4);
}

void RenderUtil::render_fill(const y::fvec2& a, const y::fvec2& b,
                             const y::fvec2& c, const y::fvec4& colour) const
{
  if (!(_native_size >= y::ivec2())) {
    return;
  }
  _gl.enable_depth(false);
  _gl.enable_blend(true);

  const GLfloat tri_data[] = {
      a[xx], a[yy], b[xx], b[yy], c[xx], c[yy]};

  auto tri_buffer = _gl.make_unique_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, tri_data, sizeof(tri_data));

  _draw_program->bind();
  _draw_program->bind_attribute("pixels", *tri_buffer);
  _draw_program->bind_uniform("colour", colour);
  bind_pixel_uniforms(*_draw_program);
  _quad_element->draw_elements(GL_TRIANGLES, 3);
}

void RenderUtil::irender_fill(const y::ivec2& origin, const y::ivec2& size,
                              const y::fvec4& colour) const
{
  render_fill(y::fvec2(origin), y::fvec2(size), colour);
}

void RenderUtil::irender_fill(const y::ivec2& a, const y::ivec2& b,
                              const y::ivec2& c, const y::fvec4& colour) const
{
  render_fill(y::fvec2(a), y::fvec2(b), y::fvec2(c), colour);
}

void RenderUtil::render_line(const y::fvec2& a, const y::fvec2& b,
                             const y::fvec4& colour) const
{
  render_lines({{a, b}}, colour);
}

void RenderUtil::render_lines(const y::vector<line>& lines,
                              const y::fvec4& colour) const
{
  if (!(_native_size >= y::ivec2())) {
    return;
  }
  _gl.enable_depth(false);
  _gl.enable_blend(true);

  y::vector<GLfloat> tri_data;
  y::vector<GLushort> element_data;

  y::size i = 0;
  for (const line& x : lines) {
    y::fvec2 normal =
        y::fvec2::from_angle((x.b - x.a).angle() + float(y::pi / 2));

    y::write_vector(tri_data, 12 * i++, {
        x.a[xx], x.a[yy],
        (x.a + normal)[xx], (x.a + normal)[yy],
        x.b[xx], x.b[yy],
        (x.a + normal)[xx], (x.a + normal)[yy],
        x.b[xx], x.b[yy],
        (x.b + normal)[xx], (x.b + normal)[yy]});
  }
  for (i = 0; i < 6 * lines.size(); ++i) {
    element_data.emplace_back(i);
  }

  auto tri_buffer = _gl.make_unique_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, tri_data);
  auto element_buffer = _gl.make_unique_buffer<GLushort, 1>(
      GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, element_data);

  _draw_program->bind();
  _draw_program->bind_attribute("pixels", *tri_buffer);
  _draw_program->bind_uniform("colour", colour);
  bind_pixel_uniforms(*_draw_program);
  element_buffer->draw_elements(GL_TRIANGLES, element_data.size());
}

void RenderUtil::render_outline(const y::fvec2& origin, const y::fvec2& size,
                                const y::fvec4& colour) const
{
  // In order to keep lines 1 pixel wide we need to divide by the scale.
  float pixel = 1.f / _scale;
  render_fill(origin, {size[xx] - pixel, pixel}, colour);
  render_fill(origin + y::fvec2{0.f, pixel},
              {pixel, size[yy] - pixel}, colour);
  render_fill(origin + y::fvec2{pixel, size[yy] - pixel},
              {size[xx] - pixel, pixel}, colour);
  render_fill(origin + y::fvec2{size[xx] - pixel, 0.f},
              {pixel, size[yy] - pixel}, colour);
}

void RenderUtil::irender_outline(const y::ivec2& origin, const y::ivec2& size,
                                 const y::fvec4& colour) const
{
  render_outline(y::fvec2(origin), y::fvec2(size), colour);
}

void RenderUtil::render_batch(
    const GlTexture2D& sprite, const y::ivec2& frame_size,
    const RenderBatch::batched_sprite_list& list) const
{
  if (!(_native_size >= y::ivec2()) ||
      !(frame_size >= y::ivec2()) || list.empty()) {
    return;
  }
  _gl.enable_depth(true);
  _gl.enable_blend(true);

  y::size length = list.size();
  for (y::size i = 0; i < length; ++i) {
    const RenderBatch::batched_sprite& s = list[i];
    float left = s.left;
    float top = s.top;

    // Vertex attribute divisor buffers would be nice for this kind of thing,
    // but it's not available until around OpenGL 4.3 which is kind of recent
    // to target.
    y::write_vector<float, y::vector<y::int32>>(_pixels_data, 8 * i, {
        -frame_size[xx] / 2, -frame_size[yy] / 2,
        frame_size[xx] / 2, -frame_size[yy] / 2,
        -frame_size[xx] / 2, frame_size[yy] / 2,
        frame_size[xx] / 2, frame_size[yy] / 2});
    y::write_vector(_rotation_data, 4 * i, {
        s.rotation, s.rotation, s.rotation, s.rotation});
    y::write_vector(_origin_data, 8 * i, {
        left, top, left, top,
        left, top, left, top});
    y::write_vector(_frame_index_data, 8 * i, {
        s.frame_x, s.frame_y, s.frame_x, s.frame_y,
        s.frame_x, s.frame_y, s.frame_x, s.frame_y});
    y::write_vector(_depth_data, 4 * i, {
        s.depth, s.depth, s.depth, s.depth});
    y::write_vector(_colour_data, 16 * i, {
        s.colour[rr], s.colour[gg], s.colour[bb], s.colour[aa],
        s.colour[rr], s.colour[gg], s.colour[bb], s.colour[aa],
        s.colour[rr], s.colour[gg], s.colour[bb], s.colour[aa],
        s.colour[rr], s.colour[gg], s.colour[bb], s.colour[aa]});
  }

  _pixels_buffer->reupload_data(_pixels_data);
  _rotation_buffer->reupload_data(_rotation_data);
  _frame_index_buffer->reupload_data(_frame_index_data);
  _origin_buffer->reupload_data(_origin_data);
  _depth_buffer->reupload_data(_depth_data);
  _colour_buffer->reupload_data(_colour_data);

  if (_element_data.size() / 6 < length) {
    for (y::size i = _element_data.size() / 6; i < length; ++i) {
      y::write_vector(_element_data, 6 * i, {
          GLushort(0 + 4 * i), GLushort(1 + 4 * i), GLushort(2 + 4 * i),
          GLushort(1 + 4 * i), GLushort(2 + 4 * i), GLushort(3 + 4 * i)});
    }
    _element_buffer->reupload_data(_element_data);
  }

  _sprite_program->bind();
  _sprite_program->bind_attribute("pixels", *_pixels_buffer);
  _sprite_program->bind_attribute("rotation", *_rotation_buffer);
  _sprite_program->bind_attribute("origin", *_origin_buffer);
  _sprite_program->bind_attribute("frame_index", *_frame_index_buffer);
  _sprite_program->bind_attribute("depth", *_depth_buffer);
  _sprite_program->bind_attribute("colour", *_colour_buffer);

  y::ivec2 v = sprite.get_size() / frame_size;
  _sprite_program->bind_uniform("sprite", sprite);
  _sprite_program->bind_uniform("frame_size", frame_size);
  _sprite_program->bind_uniform("frame_count", v);
  bind_pixel_uniforms(*_sprite_program);
  _element_buffer->draw_elements(GL_TRIANGLES, 6 * length);
}

void RenderUtil::render_batch(const RenderBatch& batch) const
{
  for (const auto& pair : batch.get_map()) {
    render_batch(pair.first.sprite, pair.first.frame_size, pair.second);
  }
}

void RenderUtil::render_sprite(
    const GlTexture2D& sprite, const y::ivec2& frame_size,
    const y::fvec2& origin, const y::ivec2& frame,
    float depth, float rotation, const y::fvec4& colour) const
{
  RenderBatch batch;
  batch.add_sprite(sprite, frame_size,
                   origin, frame, depth, rotation, colour);
  render_batch(batch);
}

void RenderUtil::irender_sprite(
    const GlTexture2D& sprite, const y::ivec2& frame_size,
    const y::ivec2& origin, const y::ivec2& frame,
    float depth, const y::fvec4& colour) const
{
  render_sprite(sprite, frame_size, y::fvec2(origin),
                frame, depth, 0.f, colour);
}

y::ivec2 RenderUtil::from_grid(const y::ivec2& grid)
{
  return grid * font_size;
}

y::ivec2 RenderUtil::to_grid(const y::ivec2& pixels)
{
  return pixels / font_size;
}

void RenderUtil::bind_pixel_uniforms(const GlProgram& program) const
{
  program.bind_uniform("resolution", _native_size);
  program.bind_uniform("translation", _translation);
  program.bind_uniform("scale", y::fvec2{_scale, _scale});
}

const y::ivec2 RenderUtil::font_size{
    RenderUtil::font_width, RenderUtil::font_height};
