#include "gl_util.h"
#include "physical_filesystem.h"
#include "window.h"

#include <SFML/Window.hpp>

int main(int argc, char** argv)
{
  Window window("Crunk Yugen", 24, 640, 360, true, true);
  PhysicalFilesystem filesystem("data");
  GlUtil gl(filesystem, window, 640, 360);
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
      gl.make_texture("bg0.png"),
      gl.make_texture("bg1.png"),
      gl.make_texture("bg2.png")};

  GLuint vertex_shader = gl.make_shader(
      "shaders/hello.v.glsl", GL_VERTEX_SHADER);
  GLuint fragment_shader = gl.make_shader(
      "shaders/hello.f.glsl", GL_FRAGMENT_SHADER);
  y::vector<GLuint> shaders;
  shaders.push_back(vertex_shader);
  shaders.push_back(fragment_shader);
  GLuint program = gl.make_program(shaders);

  GLint uniform_textures[] = {
      glGetUniformLocation(program, "textures[0]"),
      glGetUniformLocation(program, "textures[1]")};
  GLint uniform_fade_factor = glGetUniformLocation(program, "fade_factor");
  GLint attribute_position = glGetAttribLocation(program, "position");

  glUseProgram(program);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textures[1]);
  glUniform1i(uniform_textures[0], 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, textures[2]);
  glUniform1i(uniform_textures[1], 1);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(attribute_position, 2, GL_FLOAT, GL_FALSE,
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
    gl.bind_window();
    glUniform1f(uniform_fade_factor, fade_factor);

    glEnableVertexAttribArray(attribute_position);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
    glDisableVertexAttribArray(attribute_position);

    window.display();
  }
  return 0;
}
