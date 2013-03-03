#include "gl_util.h"
#include "filesystem.h"
#include "window.h"

#include <SFML/Graphics.hpp>
#include <iostream>

bool GlHandle::operator==(const GlHandle& handle) const
{
  return _handle == handle._handle;
}

bool GlHandle::operator!=(const GlHandle& handle) const
{
  return !operator==(handle);
}

GlHandle::operator bool_type() const
{
  return _handle ? &GlHandle::no_bool_comparison : 0;
}

GlHandle::GlHandle(GLuint handle)
  : _handle(handle)
{
}

GLuint GlHandle::get_handle() const
{
  return _handle;
}

GlUtil::GlUtil(const Filesystem& filesystem, const Window& window,
               GLsizei width, GLsizei height)
  : _setup_ok(false)
  , _filesystem(filesystem)
  , _window(window)
  , _framebuffer(0)
  , _framebuffer_texture(0)
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

  glGenTextures(1, &_framebuffer_texture);
  glBindTexture(GL_TEXTURE_2D, _framebuffer_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
               0, GL_RGB, GL_UNSIGNED_BYTE, 0);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, _framebuffer_texture, 0);
  GLenum draw_buffers = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(1, &draw_buffers);

  GLuint framebuffer_depth;
  glGenRenderbuffers(1, &framebuffer_depth);
  glBindRenderbuffer(GL_RENDERBUFFER, framebuffer_depth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, framebuffer_depth);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer isn't complete" << std::endl;
    return;
  }

  y::string_vector shaders;
  _filesystem.list_pattern(shaders, "shaders/**.v.glsl");
  _filesystem.list_pattern(shaders, "shaders/**.f.glsl");
  _filesystem.list_pattern(shaders, "shaders/**.v");
  _filesystem.list_pattern(shaders, "shaders/**.f");

  _setup_ok = true;
  for (const y::string& shader : shaders) {
    if (!make_shader(shader)) {
      _setup_ok = false;
    }
  }
}

GlUtil::~GlUtil()
{
  for (GLuint buffer : _buffer_set) {
    glDeleteBuffers(1, &buffer);
  }
  for (const auto& pair : _texture_map) {
    GLuint handle = pair.second.get_handle();
    glDeleteTextures(1, &handle);
  }
  for (const auto& pair : _shader_map) {
    glDeleteShader(pair.second.get_handle());
  }
  for (const auto& pair : _program_map) {
    glDeleteProgram(pair.second.get_handle());
  }
}

GlUtil::operator bool_type() const
{
  return _setup_ok ? &GlUtil::no_bool_comparison : 0;
}

GlTexture GlUtil::make_texture(const y::string& filename)
{
  y::string data;
  _filesystem.read_file(data, filename);
  if (data.empty()) {
    std::cerr << "Couldn't read file " << filename << std::endl;
    return GlTexture(0, 0, 0);
  }

  sf::Image image;
  if (!image.loadFromMemory(&data[0], data.length())) {
    std::cerr << "Couldn't load image " << filename << std::endl;
    return GlTexture(0, 0, 0);
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

  auto it = _texture_map.find(filename);
  if (it != _texture_map.end()) {
    GLuint handle = it->second.get_handle();
    glDeleteTextures(1, &handle);
    _texture_map.erase(it);
  }
  GlTexture r(texture, image.getSize().x, image.getSize().y);
  _texture_map.insert(y::make_pair(filename, r));
  return r;
}

GlTexture GlUtil::get_texture(const y::string& filename) const
{
  auto it = _texture_map.find(filename);
  return it == _texture_map.end() ? GlTexture(0, 0, 0) : it->second;
}

void GlUtil::delete_texture(const y::string& filename)
{
  auto it = _texture_map.find(filename);
  if (it != _texture_map.end()) {
    GLuint handle = it->second.get_handle();
    glDeleteTextures(1, &handle);
    _texture_map.erase(it);
  }
}

GlShader GlUtil::make_shader(const y::string& filename, GLenum type)
{
  y::string data;
  _filesystem.read_file(data, filename);
  if (data.empty()) {
    std::cerr << "Couldn't read file " << filename << std::endl;
    return GlShader(0);
  }

  if (!type) {
    if (filename.substr(filename.length() - 7) == ".v.glsl" ||
        filename.substr(filename.length() - 2) == ".v") {
      type = GL_VERTEX_SHADER;
    }
    else if (filename.substr(filename.length() - 7) == ".f.glsl" ||
             filename.substr(filename.length() - 2) == ".f") {
      type = GL_FRAGMENT_SHADER;
    }
  }
  GLuint shader = glCreateShader(type);
  const char* char_data = data.c_str();
  GLint lengths[] = {(GLint)data.length()};
  glShaderSource(shader, 1, (const GLchar**)&char_data, lengths);
  glCompileShader(shader);

  GLint ok;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (ok) {
    auto it = _shader_map.find(filename);
    if (it != _shader_map.end()) {
      glDeleteShader(it->second.get_handle());
      _shader_map.erase(it);
    }
    GlShader r(shader);
    _shader_map.insert(y::make_pair(filename, r));
    return r;
  }

  std::cerr << "Shader " << filename << " failed compilation" << std::endl;
  GLint log_length;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
  y::unique<char[]> log(new char[log_length]);
  glGetShaderInfoLog(shader, log_length, 0, log.get());
  std::cerr << log.get();
  glDeleteShader(shader);
  return GlShader(0);
}

GlShader GlUtil::get_shader(const y::string& filename) const
{
  auto it = _shader_map.find(filename);
  return it == _shader_map.end() ? GlShader(0) : it->second;
}

void GlUtil::delete_shader(const y::string& filename)
{
  auto it = _shader_map.find(filename);
  if (it != _shader_map.end()) {
    glDeleteShader(it->second.get_handle());
    _shader_map.erase(it);
  }
}

GlProgram GlUtil::make_program(const y::string_vector& shaders)
{
  y::string_vector sort;
  std::copy(shaders.begin(), shaders.end(), std::back_inserter(sort));
  std::sort(sort.begin(), sort.end());

  y::string hash;
  for (const y::string& s : sort) {
    hash += s + '\n';
  }

  GLuint program = glCreateProgram();
  for (const y::string& shader : shaders) {
    glAttachShader(program, get_shader(shader).get_handle());
  }
  glLinkProgram(program);

  GLint ok;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (ok) {
    auto it = _program_map.find(hash);
    if (it != _program_map.end()) {
      glDeleteProgram(it->second.get_handle());
      _program_map.erase(it);
    }
    GlProgram r(program);
    _program_map.insert(y::make_pair(hash, r));
    return r;
  }

  std::cerr << "Program failed linking" << std::endl;
  GLint log_length;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
  y::unique<char[]> log(new char[log_length]);
  glGetProgramInfoLog(program, log_length, 0, log.get());
  std::cerr << log.get();
  glDeleteProgram(program);
  return GlProgram(0);
}

GlProgram GlUtil::get_program(const y::string_vector& shaders) const
{
  y::string_vector sort;
  std::copy(shaders.begin(), shaders.end(), std::back_inserter(sort));
  std::sort(sort.begin(), sort.end());

  y::string hash;
  for (const y::string& s : sort) {
    hash += s + '\n';
  }

  auto it = _program_map.find(hash);
  return it == _program_map.end() ? GlProgram(0) : it->second;
}

void GlUtil::delete_program(const y::string_vector& shaders)
{
  y::string_vector sort;
  std::copy(shaders.begin(), shaders.end(), std::back_inserter(sort));
  std::sort(sort.begin(), sort.end());

  y::string hash;
  for (const y::string& s : sort) {
    hash += s + '\n';
  }

  auto it = _program_map.find(hash);
  if (it != _program_map.end()) {
    glDeleteProgram(it->second.get_handle());
    _program_map.erase(it);
  }
}

GlTexture GlUtil::get_framebuffer() const
{
  return GlTexture(_framebuffer_texture,
                   _framebuffer_width, _framebuffer_height);
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

GlTexture::GlTexture(GLuint handle, y::size width, y::size height)
  : GlHandle(handle)
  , _width(width)
  , _height(height)
{
}

void GlTexture::bind(GLenum target) const
{
  glActiveTexture(target);
  glBindTexture(GL_TEXTURE_2D, get_handle());
}

y::size GlTexture::get_width() const
{
  return _width;
}

y::size GlTexture::get_height() const
{
  return _height;
}

GlShader::GlShader(GLuint handle)
  : GlHandle(handle)
{
}

GlProgram::GlProgram(GLuint handle)
  : GlHandle(handle)
{
}

void GlProgram::bind() const
{
  glUseProgram(get_handle());
}

bool GlProgram::check_name_exists(bool attribute, const y::string& name,
                                  bool array, y::size index,
                                  GLenum& type_output) const
{
  GLint name_count;
  glGetProgramiv(
      get_handle(), attribute ? GL_ACTIVE_ATTRIBUTES : GL_ACTIVE_UNIFORMS,
      &name_count);

  GLint name_length;
  glGetProgramiv(
      get_handle(),
      attribute ? GL_ACTIVE_ATTRIBUTE_MAX_LENGTH : GL_ACTIVE_UNIFORM_MAX_LENGTH,
      &name_length);

  y::unique<char[]> buffer(new char[name_length]);
  GLint array_size;
  for (GLint i = 0; i < name_count; ++i) {
    if (attribute) {
      glGetActiveAttrib(get_handle(), i, name_length, y::null,
                        &array_size, &type_output, buffer.get());
    }
    else {
      glGetActiveUniform(get_handle(), i, name_length, y::null,
                         &array_size, &type_output, buffer.get());
    }
    if (name == buffer.get() &&
        ((!array && array_size == 1) ||
         (array && signed(index) < array_size))) {
      return true;
    }
  }
  return false;
}

bool GlProgram::check_match(bool attribute, const y::string& name,
                            bool array, y::size index,
                            GLenum type, y::size length) const
{
  GLenum name_type;
  if (!check_name_exists(attribute, name, array, index, name_type)) {
    std::cerr << "Undefined " <<
        (attribute ? "attribute" : "uniform") << " " << name;
    if (array) {
      std::cerr << "[" << index << "]";
    } 
    std::cerr << std::endl;
    return false;
  }
  // TODO: figure out type stuff
  return true;
}
