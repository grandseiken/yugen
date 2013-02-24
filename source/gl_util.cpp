#include "gl_util.h"
#include "filesystem.h"
#include "window.h"

#include <SFML/Graphics.hpp>
#include <iostream>

GlUtil::GlUtil(const Filesystem& filesystem, const Window& window,
               GLsizei width, GLsizei height)
  : _setup_ok(false)
  , _filesystem(filesystem)
  , _window(window)
  , _framebuffer(0)
  , _framebuffer_width(width)
  , _framebuffer_height(height)
{
  GLenum ok = glewInit();
  if (ok != GLEW_OK) {
    std::cerr << "Couldn't initialise GLEW: " <<
        glewGetErrorString(ok) << std::endl;
    return;
  }

  if (!GLEW_VERSION_2_1) {
    std::cerr << "OpenGL 2.1 not available" << std::endl;
    return;
  }

  if (!GLEW_EXT_framebuffer_object) {
    std::cerr << "OpenGL framebuffer object not available" << std::endl;
    return;
  }

  glGenFramebuffers(1, &_framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
  GLuint framebuffer_texture;
  GLuint framebuffer_depth;

  glGenTextures(1, &framebuffer_texture);
  glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
               0, GL_RGB, GL_UNSIGNED_BYTE, 0);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, framebuffer_texture, 0);
  GLenum draw_buffers = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(1, &draw_buffers);

  glGenRenderbuffers(1, &framebuffer_depth);
  glBindRenderbuffer(GL_RENDERBUFFER, framebuffer_depth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, framebuffer_depth);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer isn't complete" << std::endl;
    return;
  }

  _setup_ok = true;
}

GlUtil::~GlUtil()
{
}

bool GlUtil::setup_ok() const
{
  return _setup_ok;
}

GLuint GlUtil::make_buffer(GLenum target, GLenum usage_hint,
                           const void* data, GLsizei size) const
{
  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(target, buffer);
  glBufferData(target, size, data, usage_hint);
  return buffer;
}

GLuint GlUtil::make_texture(const y::string& filename) const
{
  y::string data;
  _filesystem.read_file(data, filename);
  if (data.empty()) {
    std::cerr << "Couldn't read file " << filename << std::endl;
    return 0;
  }

  sf::Image image;
  if (!image.loadFromMemory(&data[0], data.length())) {
    std::cerr << "Couldn't load image " << filename << std::endl;
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

GLuint GlUtil::make_shader(const y::string& filename, GLenum type) const
{
  y::string data;
  _filesystem.read_file(data, filename);
  if (data.empty()) {
    std::cerr << "Couldn't read file " << filename << std::endl;
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

  std::cerr << "Shader " << filename << " failed compilation" << std::endl;
  GLint log_length;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
  y::unique<char[]> log(new char[log_length]);
  glGetShaderInfoLog(shader, log_length, 0, log.get());
  std::cerr << log.get();
  glDeleteShader(shader);
  return 0;
}

GLuint GlUtil::make_program(const y::vector<GLuint>& shaders) const
{
  GLuint program = glCreateProgram();
  for (GLuint shader : shaders) {
    glAttachShader(program, shader);
  }
  glLinkProgram(program);

  GLint ok;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (ok) {
    return program;
  }

  std::cerr << "Program failed linking" << std::endl;
  GLint log_length;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
  y::unique<char[]> log(new char[log_length]);
  glGetProgramInfoLog(program, log_length, 0, log.get());
  std::cerr << log.get();
  glDeleteProgram(program);
  return 0;
}

void GlUtil::bind_framebuffer() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
  glViewport(0, 0, _framebuffer_width, _framebuffer_height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GlUtil::bind_window() const
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  const Resolution& mode = _window.get_mode();
  glViewport(0, 0, mode.width, mode.height);
  glClear(GL_COLOR_BUFFER_BIT);
}
