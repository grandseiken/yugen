#include "databank.h"
#include "gl_util.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "window.h"

#include <SFML/Window.hpp>

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
  
  const GLfloat vertex_data[] = {
      -1.f, -1.f,
       1.f, -1.f,
      -1.f,  1.f,
       1.f,  1.f};

  auto vertex_buffer = gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertex_data, sizeof(vertex_data));

  GlTexture textures[3] = {
      gl.make_texture("/bg0.png"),
      gl.make_texture("/bg1.png"),
      gl.make_texture("/bg2.png")};

  GlProgram hello_program = gl.make_program({
      "/shaders/hello.v.glsl",
      "/shaders/hello.f.glsl"});

  bool running = true;
  bool direction = true;
  GLfloat fade_factor = 0.f;

  while (running) {
    sf::Event event;
    while (window.poll_event(event)) {
      if (event.type == sf::Event::Closed ||
          (event.type == sf::Event::KeyPressed &&
           event.key.code == sf::Keyboard::Escape)) {
        running = false;
      }
    }

    if (direction) {
      fade_factor += 0.01f;
    }
    else {
      fade_factor -= 0.01f;
    }
    if (fade_factor >= 1.f || fade_factor <= 0.f) {
      direction = !direction;
    }

    gl.bind_window();
    hello_program.bind();
    hello_program.bind_attribute("position", vertex_buffer);
    hello_program.bind_uniform("fade_factor", fade_factor);
    textures[1].bind(GL_TEXTURE0);
    textures[2].bind(GL_TEXTURE1);
    hello_program.bind_uniform(0, "textures", 0);
    hello_program.bind_uniform(1, "textures", 1);
    util.quad().draw_elements(GL_TRIANGLE_STRIP, 4);

    const Resolution& screen = window.get_mode();
    util.set_resolution(screen.width, screen.height);
    util.render_text("Hello, world!", 16.0f, 16.0f,
                     0.0f, 0.0f, 0.0f, 1.0f);

    window.display();
  }
  return 0;
}
