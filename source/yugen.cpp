#include "gl_util.h"
#include "physical_filesystem.h"
#include "window.h"
#include "world.h"

#include <SFML/Window.hpp>

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

  GlTexture font = gl.make_texture("/font.png");
  const GLfloat text_data[] = {
        16.f, 16.f,
       640.f, 16.f,
        16.f, 24.f,
       640.f, 24.f};
  auto text_buffer = gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, text_data, sizeof(text_data));

  const GLfloat vertex_data[] = {
      -1.f, -1.f,
       1.f, -1.f,
      -1.f,  1.f,
       1.f,  1.f};

  const GLushort element_data[] = {0, 1, 2, 3};

  auto vertex_buffer = gl.make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertex_data, sizeof(vertex_data));

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

  shaders.clear();
  shaders.push_back("/shaders/text.v.glsl");
  shaders.push_back("/shaders/text.f.glsl");
  GlProgram text_program = gl.make_program(shaders);

  shaders.clear();
  shaders.push_back("/shaders/post.v.glsl");
  shaders.push_back("/shaders/post.f.glsl");
  GlProgram post_program = gl.make_program(shaders);

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

    framebuffer.bind();
    hello_program.bind();
    hello_program.bind_attribute("position", vertex_buffer);
    hello_program.bind_uniform("fade_factor", fade_factor);
    textures[1].bind(GL_TEXTURE0);
    textures[2].bind(GL_TEXTURE1);
    hello_program.bind_uniform(0, "textures", 0);
    hello_program.bind_uniform(1, "textures", 1);
    element_buffer.draw_elements(GL_TRIANGLE_STRIP, 4);

    const y::string str = "Hello, world!";
    text_program.bind();
    text_program.bind_attribute("pixels", text_buffer);
    text_program.bind_uniform("resolution",
        GLint(native_width), GLint(native_height));
    text_program.bind_uniform("origin",
        GLint(text_data[0]), GLint(text_data[0]));
    for (y::size i = 0; i < 1024; ++i) {
      text_program.bind_uniform(i, "string",
          GLint(i < str.length() ? str[i] : 0));
    }
    font.bind(GL_TEXTURE0);
    text_program.bind_uniform("font", 0);
    text_program.bind_uniform("font_size", GLint(8), GLint(8));
    text_program.bind_uniform("colour", 1.0f, 0.5f, 0.0f, 1.0f);
    element_buffer.draw_elements(GL_TRIANGLE_STRIP, 4);

    const Resolution& screen = window.get_mode();
    gl.bind_window();
    post_program.bind();
    post_program.bind_attribute("position", vertex_buffer);
    post_program.bind_uniform("integral_scale_lock", true);
    post_program.bind_uniform("native_res",
        GLint(native_width), GLint(native_height));
    post_program.bind_uniform("screen_res",
        GLint(screen.width), GLint(screen.height));
    framebuffer.get_texture().bind(GL_TEXTURE0);
    post_program.bind_uniform("framebuffer", 0);
    element_buffer.draw_elements(GL_TRIANGLE_STRIP, 4);

    window.display();
  }
  return 0;
}
