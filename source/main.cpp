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
  if (!gl.setup_ok()) {
    return 1;
  }

  const GLfloat vertex_data[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
  const GLushort element_data[] = {0, 1, 2, 3};

  GLuint vertex_buffer = gl.make_buffer(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertex_data, sizeof(vertex_data));
  GLuint element_buffer = gl.make_buffer(
      GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW,
      element_data, sizeof(element_data));

  GLuint textures[3] = {
      gl.make_texture("/bg0.png"),
      gl.make_texture("/bg1.png"),
      gl.make_texture("/bg2.png")};

  y::string_vector shaders;
  shaders.push_back("/shaders/hello.v.glsl");
  shaders.push_back("/shaders/hello.f.glsl");
  GLuint hello_program = gl.make_program(shaders);

  GLint hello_attribute_position =
      glGetAttribLocation(hello_program, "position");

  shaders.clear();
  shaders.push_back("/shaders/post.v.glsl");
  shaders.push_back("/shaders/post.f.glsl");
  GLuint post_program = gl.make_program(shaders);

  GLint post_attribute_position =
      glGetAttribLocation(post_program, "position");

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(hello_attribute_position, 2, GL_FLOAT, GL_FALSE,
                        sizeof(GLfloat) * 2, (void*)0);
  glVertexAttribPointer(post_attribute_position, 2, GL_FLOAT, GL_FALSE,
                        sizeof(GLfloat) * 2, (void*)0);

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
    glUseProgram(hello_program);
    glUniform1f(
        glGetUniformLocation(hello_program, "fade_factor"), fade_factor);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glUniform1i(glGetUniformLocation(hello_program, "textures[0]"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    glUniform1i(glGetUniformLocation(hello_program, "textures[1]"), 1);

    glEnableVertexAttribArray(hello_attribute_position);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
    glDisableVertexAttribArray(hello_attribute_position);

    gl.bind_window();
    glUseProgram(post_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gl.get_framebuffer());
    glUniform1i(glGetUniformLocation(post_program, "framebuffer"), 0);

    glEnableVertexAttribArray(post_attribute_position);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
    glDisableVertexAttribArray(post_attribute_position);

    window.display();
  }
  return 0;
}
