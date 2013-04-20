#include "yugen.h"

#include "gl_util.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "window.h"
#include "world.h"

#include <SFML/Window.hpp>

const GLfloat vertex_data[] = {
    -1.f, -1.f,
     1.f, -1.f,
    -1.f,  1.f,
     1.f,  1.f};

Yugen::Yugen(RenderUtil& util, GlFramebuffer& framebuffer)
  : _util(util)
  , _framebuffer(framebuffer)
  , _direction(true)
  , _fade_factor(0.f)
  , _hello_program(util.get_gl().make_program({
      "/shaders/hello.v.glsl",
      "/shaders/hello.f.glsl"}))
  , _post_program(util.get_gl().make_program({
      "/shaders/post.v.glsl",
      "/shaders/post.f.glsl"}))
  , _textures({
      util.get_gl().make_texture("/bg0.png"),
      util.get_gl().make_texture("/bg1.png"),
      util.get_gl().make_texture("/bg2.png")})
  , _vertex_buffer(util.get_gl().make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertex_data, sizeof(vertex_data)))
{
}

void Yugen::event(const sf::Event& e)
{
  if (e.type == sf::Event::KeyPressed &&
      e.key.code == sf::Keyboard::Escape) {
    end();
  }
}

void Yugen::update()
{
  // TODO: window framerate limiting is untrustworthy. Lock to 60 here.
  _fade_factor += _direction ? 0.01f : -0.01f;
  if (_fade_factor >= 1.f || _fade_factor <= 0.f) {
    _direction = !_direction;
  }

  _measurements.push_back(_clock.restart().asMilliseconds());
  if (_measurements.size() > samples) {
    _measurements.erase(_measurements.begin());
  }
}

void Yugen::draw() const
{
  y::size total = 0;
  if (!_measurements.empty()) {
    for (y::size m : _measurements) {
      total += m;
    }
    total /= samples;
  }

  _framebuffer.bind(true);
  _hello_program.bind();
  _hello_program.bind_attribute("position", _vertex_buffer);
  _hello_program.bind_uniform("fade_factor", _fade_factor);
  _textures[1].bind(GL_TEXTURE0);
  _textures[2].bind(GL_TEXTURE1);
  _hello_program.bind_uniform(0, "textures", 0);
  _hello_program.bind_uniform(1, "textures", 1);
  _util.quad().draw_elements(GL_TRIANGLE_STRIP, 4);

  _util.set_resolution(_framebuffer.get_width(), _framebuffer.get_height());
  if (total) {
    y::sstream ss;
    ss << total << " ticks (" << (1000.f / total) << " fps)";
    _util.render_text(ss.str(), 16.f, 16.f, 0.f, 0.f, 0.f, 1.f);
  }

  const Resolution& screen = _util.get_window().get_mode();
  _util.get_gl().bind_window(true);
  _post_program.bind();
  _post_program.bind_attribute("position", _vertex_buffer);
  _post_program.bind_uniform("integral_scale_lock", true);
  _post_program.bind_uniform("native_res",
      GLint(_framebuffer.get_width()), GLint(_framebuffer.get_height()));
  _post_program.bind_uniform("screen_res",
      GLint(screen.width), GLint(screen.height));
  _framebuffer.get_texture().bind(GL_TEXTURE0);
  _post_program.bind_uniform("framebuffer", 0);
  _util.quad().draw_elements(GL_TRIANGLE_STRIP, 4);
}

int main(int argc, char** argv)
{
  World world;

  const y::size native_width = 640;
  const y::size native_height = 360;

  Window window("Crunk Yugen", 24, native_width, native_height, true, false);
  PhysicalFilesystem filesystem("data");
  GlUtil gl(filesystem, window);
  if (!gl) {
    return 1;
  }
  GlFramebuffer framebuffer = gl.make_framebuffer(native_width, native_height);
  RenderUtil util(gl);

  ModalStack stack;
  stack.push(y::move_unique(new Yugen(util, framebuffer)));
  stack.run(window);
  return 0;
}
