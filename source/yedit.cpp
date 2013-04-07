#include "databank.h"
#include "gl_util.h"
#include "modal.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "window.h"

#include <SFML/Window.hpp>

const GLfloat vertex_data[] = {
    -1.f, -1.f,
     1.f, -1.f,
    -1.f,  1.f,
     1.f,  1.f};

class Editor : public Modal {
public:

  Editor(Window& window, GlUtil& gl, RenderUtil& util)
    : _window(window)
    , _gl(gl)
    , _util(util)
    , _direction(true)
    , _fade_factor(0.f)
    , _hello_program(gl.make_program({
        "/shaders/hello.v.glsl",
        "/shaders/hello.f.glsl"}))
    , _textures({
        gl.make_texture("/bg0.png"),
        gl.make_texture("/bg1.png"),
        gl.make_texture("/bg2.png")})
    , _vertex_buffer(gl.make_buffer<GLfloat, 2>(
        GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertex_data, sizeof(vertex_data))) {}
  virtual ~Editor() {}

  virtual void event(const sf::Event& e)
  {
    if (e.type == sf::Event::Closed ||
        (e.type == sf::Event::KeyPressed &&
         e.key.code == sf::Keyboard::Escape)) {
      end();
    }
  }

  virtual void update()
  {
    if (_direction) {
      _fade_factor += 0.01f;
    }
    else {
      _fade_factor -= 0.01f;
    }
    if (_fade_factor >= 1.f || _fade_factor <= 0.f) {
      _direction = !_direction;
    }
  }

  virtual void draw() const
  {
    _gl.bind_window();
    _hello_program.bind();
    _hello_program.bind_attribute("position", _vertex_buffer);
    _hello_program.bind_uniform("fade_factor", _fade_factor);
    _textures[1].bind(GL_TEXTURE0);
    _textures[2].bind(GL_TEXTURE1);
    _hello_program.bind_uniform(0, "textures", 0);
    _hello_program.bind_uniform(1, "textures", 1);
    _util.quad().draw_elements(GL_TRIANGLE_STRIP, 4);

    const Resolution& screen = _window.get_mode();
    _util.set_resolution(screen.width, screen.height);
    _util.render_text("Hello, world!", 16.0f, 16.0f,
                      0.0f, 0.0f, 0.0f, 1.0f);
  }

private:

  Window& _window;
  GlUtil& _gl;
  RenderUtil& _util;

  bool _direction;
  GLfloat _fade_factor;

  GlProgram _hello_program;
  GlTexture _textures[3];
  GlBuffer<GLfloat, 2> _vertex_buffer;

};

int main(int argc, char** argv)
{
  Window window("Crunk Yedit", 24, 0, 0, false, true);
  PhysicalFilesystem filesystem("data");
  GlUtil gl(filesystem, window);
  if (!gl) {
    return 1;
  }
  Databank databank(filesystem, gl);
  RenderUtil util(gl);
  
  ModalStack stack;
  stack.push(y::move_unique(new Editor(window, gl, util)));
  stack.run(window);
  return 0;
}
