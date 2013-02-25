#include "gl_util.h"
#include "physical_filesystem.h"
#include "window.h"

#include <SFML/Window.hpp>

int main(int argc, char** argv)
{
  const y::size native_width = 640;
  const y::size native_height = 360;

  Window window("Crunk Yugen", 24, native_width, native_height, true, true);
  PhysicalFilesystem filesystem("data");
  GlUtil gl(filesystem, window, native_width, native_height);
  if (!gl) {
    return 1;
  }

  const GLfloat vertex_data[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
  auto vertex_buffer = gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertex_data, sizeof(vertex_data));

  const GLushort element_data[] = {0, 1, 2, 3};
  auto element_buffer = gl.make_buffer<GLushort, 1>(
      GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
      element_data, sizeof(element_data));

  GlTexture textures[3] = {
      gl.make_texture("/bg0.png"),
      gl.make_texture("/bg1.png"),
      gl.make_texture("/bg2.png")};

  y::string_vector shaders;
  shaders.push_back("/shaders/hello.v.glsl");
  shaders.push_back("/shaders/hello.f.glsl");
  GlProgram hello_program = gl.make_program(shaders);
  hello_program.bind_attribute("position", vertex_buffer);

  shaders.clear();
  shaders.push_back("/shaders/post.v.glsl");
  shaders.push_back("/shaders/post.f.glsl");
  GlProgram post_program = gl.make_program(shaders);
  post_program.bind_attribute("position", vertex_buffer);

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

    gl.bind_framebuffer();
    hello_program.bind();
    hello_program.bind_uniform("fade_factor", fade_factor);
    textures[1].bind(GL_TEXTURE0);
    textures[2].bind(GL_TEXTURE1);
    hello_program.bind_uniform("textures[0]", 0);
    hello_program.bind_uniform("textures[1]", 1);
    element_buffer.draw_elements(GL_TRIANGLE_STRIP, 4);

    gl.bind_window();
    post_program.bind();
    gl.get_framebuffer().bind(GL_TEXTURE0);
    post_program.bind_uniform("framebuffer", 0);
    element_buffer.draw_elements(GL_TRIANGLE_STRIP, 4);

    window.display();
  }
  return 0;
}
