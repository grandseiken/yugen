#include "gl_util.h"
#include "modal.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "window.h"
#include "world.h"

#include <SFML/System.hpp>
#include <SFML/Window.hpp>

class Yugen : public Modal {
public:

  Yugen(Window& window, GlUtil& gl, RenderUtil& util,
        GlFramebuffer& framebuffer);
  virtual ~Yugen() {}

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  static const y::size samples = 16;
  sf::Clock _clock;
  y::vector<y::size> _measurements;

  Window& _window;
  GlUtil& _gl;
  RenderUtil& _util;
  GlFramebuffer& _framebuffer;

  bool _direction;
  GLfloat _fade_factor;

  GlProgram _hello_program;
  GlProgram _post_program;
  GlTexture _textures[3];
  GlBuffer<GLfloat, 2> _vertex_buffer;

};

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
  stack.push(y::move_unique(new Yugen(window, gl, util, framebuffer)));
  stack.run(window);
  return 0;
}

const GLfloat vertex_data[] = {
    -1.f, -1.f,
     1.f, -1.f,
    -1.f,  1.f,
     1.f,  1.f};

Yugen::Yugen(Window& window, GlUtil& gl, RenderUtil& util,
             GlFramebuffer& framebuffer)
  : _window(window)
  , _gl(gl)
  , _util(util)
  , _framebuffer(framebuffer)
  , _direction(true)
  , _fade_factor(0.f)
  , _hello_program(gl.make_program({
      "/shaders/hello.v.glsl",
      "/shaders/hello.f.glsl"}))
  , _post_program(gl.make_program({
      "/shaders/post.v.glsl",
      "/shaders/post.f.glsl"}))
  , _textures({
      gl.make_texture("/bg0.png"),
      gl.make_texture("/bg1.png"),
      gl.make_texture("/bg2.png")})
  , _vertex_buffer(gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertex_data, sizeof(vertex_data)))
{
}

void Yugen::event(const sf::Event& e)
{
  if (e.type == sf::Event::Closed ||
      (e.type == sf::Event::KeyPressed &&
       e.key.code == sf::Keyboard::Escape)) {
    end();
  }
}

void Yugen::update()
{
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

  _framebuffer.bind();
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

  const Resolution& screen = _window.get_mode();
  _gl.bind_window();
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
