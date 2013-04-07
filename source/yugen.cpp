#include "gl_util.h"
#include "physical_filesystem.h"
#include "render_util.h"
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

  GlProgram post_program = gl.make_program({
      "/shaders/post.v.glsl",
      "/shaders/post.f.glsl"});

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
    util.quad().draw_elements(GL_TRIANGLE_STRIP, 4);

    util.set_resolution(native_width, native_height);
    util.render_text("Hello, world!", 16.0f, 16.0f,
                     1.0f, 0.5f, 0.0f, 1.0f);

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
    util.quad().draw_elements(GL_TRIANGLE_STRIP, 4);

    window.display();
  }
  return 0;
}
