#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>

#include "window.h"
#include "physical_filesystem.h"

GLuint make_buffer(GLenum target, const void* data, GLsizei size,
                   GLenum usage_hint)
{
  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(target, buffer);
  glBufferData(target, size, data, usage_hint);
  return buffer;
}

GLuint make_texture(const Filesystem& filesystem, const y::string& filename)
{
  y::string data;
  filesystem.read_file(data, filename);
  if (data.empty()) {
    return 0;
  }

  sf::Image image;
  if (!image.loadFromMemory(&data[0], data.length())) {
    return 0;
  }
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.getSize().x, image.getSize().y,
               0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
  return texture;
}

GLuint make_shader(const Filesystem& filesystem, const y::string& filename,
                   GLenum type)
{
  y::string data;
  filesystem.read_file(data, filename);
  if (data.empty()) {
    return 0;
  }

  GLuint shader = glCreateShader(type);
  const char* char_data = data.c_str();
  GLint lengths[] = {(GLint)data.length()};
  glShaderSource(shader, 1, (const GLchar**)&char_data, lengths);
  glCompileShader(shader);

  GLint ok;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (ok) {
    return shader;
  }

  std::cerr << "Shader " << filename << " failed compilation";
  GLint log_length;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
  y::unique<char[]> log(new char[log_length]);
  glGetShaderInfoLog(shader, log_length, 0, log.get());
  std::cerr << log.get();
  glDeleteShader(shader);
  return 0;
}

GLuint make_program(GLuint vertex_shader, GLuint fragment_shader)
{
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  GLint ok;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (ok) {
    return program;
  }

  std::cerr << "Program failed linking";
  GLint log_length;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
  y::unique<char[]> log(new char[log_length]);
  glGetProgramInfoLog(program, log_length, 0, log.get());
  std::cerr << log.get();
  glDeleteProgram(program);
  return 0;
}

int main(int argc, char** argv)
{
  Window window("Crunk Yugen", 24, 640, 360, true, true);
  PhysicalFilesystem filesystem("data");

  glewInit();
  if (!GLEW_VERSION_2_1) {
    std::cerr << "OpenGL 2.1 not available" << std::endl;
  }

  const GLfloat vertex_data[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
  const GLushort element_data[] = {0, 1, 2, 3};

  GLuint vertex_buffer = make_buffer(
      GL_ARRAY_BUFFER, vertex_data, sizeof(vertex_data), GL_STATIC_DRAW);
  GLuint element_buffer = make_buffer(
      GL_ELEMENT_ARRAY_BUFFER, element_data, sizeof(element_data),
      GL_STATIC_DRAW);
  GLuint textures[3] = {
      make_texture(filesystem, "bg0.png"),
      make_texture(filesystem, "bg1.png"),
      make_texture(filesystem, "bg2.png")};

  GLuint vertex_shader = make_shader(
      filesystem, "shaders/hello.v.glsl", GL_VERTEX_SHADER);
  GLuint fragment_shader = make_shader(
      filesystem, "shaders/hello.f.glsl", GL_FRAGMENT_SHADER);
  GLuint program = make_program(vertex_shader, fragment_shader);

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniform1f(uniform_fade_factor, fade_factor);

    glEnableVertexAttribArray(attribute_position);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);
    glDisableVertexAttribArray(attribute_position);

    window.display();
  }
  return 0;
}
