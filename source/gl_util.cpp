#include "gl_util.h"
#include "filesystem/filesystem.h"
#include "window.h"

#include <algorithm>
#include <SFML/Graphics.hpp>

GlUtil::GlUtil(const Filesystem& filesystem, const Window& window)
  : _setup_ok(false)
  , _filesystem(filesystem)
  , _window(window)
{
  GLenum ok = glewInit();
  if (ok != GLEW_OK) {
    std::cerr << "Couldn't initialise GLEW: " <<
        glewGetErrorString(ok) << std::endl;
    return;
  }

  // OpenGL 2.1 is very old. we could potentially target 3.0 if there's any
  // reason to. Even 3.1 seems to have patchy support on Linux, though. Also,
  // Intel cards generally don't go beyond 2.1. However, the biggest change is
  // a cleaner shader language (in/out), which perhaps isn't stricly necessary.
  // Also allows multiple render-targets in a single shader pass, vertex array
  // objects, and so on.
  // http://www.opengl.org/registry/doc/GLSLangSpec.Full.1.30.10.pdf
  if (!GLEW_VERSION_2_1) {
    std::cerr << "OpenGL 2.1 not available" << std::endl;
    return;
  }

  if (!GLEW_ARB_texture_non_power_of_two) {
    std::cerr << "OpenGL non-power-of-two textures not available" << std::endl;
    return;
  }

  if (!GLEW_ARB_shading_language_100 || !GLEW_ARB_shader_objects ||
      !GLEW_ARB_vertex_shader || !GLEW_ARB_fragment_shader) {
    std::cerr << "OpenGL shaders not available" << std::endl;
    return;
  }

  if (!GLEW_EXT_framebuffer_object) {
    std::cerr << "OpenGL framebuffer object not available" << std::endl;
    return;
  }

  y::string_vector shaders;
  _filesystem.list_pattern(shaders, "/shaders/**.v.glsl");
  _filesystem.list_pattern(shaders, "/shaders/**.f.glsl");
  _filesystem.list_pattern(shaders, "/shaders/**.v");
  _filesystem.list_pattern(shaders, "/shaders/**.f");

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
  for (GLuint texture : _texture_set) {
    glDeleteTextures(1, &texture);
  }
  for (GLuint framebuffer : _framebuffer_set) {
    glDeleteFramebuffers(1, &framebuffer);
  }
  for (GLuint depth : _framebuffer_depth_set) {
    glDeleteRenderbuffers(1, &depth);
  }
  for (const auto& pair : _shader_map) {
    glDeleteShader(pair.second.get_handle());
  }
  for (const auto& pair : _program_map) {
    glDeleteProgram(pair.second.get_handle());
  }
}

GlUtil::operator bool() const
{
  return _setup_ok;
}

const Window& GlUtil::get_window() const
{
  return _window;
}

GlFramebuffer GlUtil::make_framebuffer(const y::ivec2& size,
                                       bool has_alpha, bool has_depth)
{
  GLuint framebuffer;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  GlTexture2D texture(make_texture<GLubyte>(size,
                                            has_alpha ? GL_RGBA8 : GL_RGB,
                                            has_alpha ? GL_RGBA : GL_RGB,
                                            y::null));
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, texture.get_handle(), 0);
  GLenum draw_buffers = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(1, &draw_buffers);

  GLuint depth = 0;
  if (has_depth) {
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                          size[xx], size[yy]);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth);
  }

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer isn't complete" << std::endl;
    delete_texture(texture);
    glDeleteFramebuffers(1, &framebuffer);
    if (has_depth) {
      glDeleteRenderbuffers(1, &depth);
    }
    return GlFramebuffer();
  }

  _framebuffer_set.insert(framebuffer);
  if (has_depth) {
    _framebuffer_depth_set.insert(depth);
  }
  return GlFramebuffer(framebuffer, texture, depth);
}

GlUnique<GlFramebuffer> GlUtil::make_unique_framebuffer(
    const y::ivec2& size, bool has_alpha, bool has_depth)
{
  return make_unique(make_framebuffer(size, has_alpha, has_depth));
}

void GlUtil::delete_framebuffer(const GlFramebuffer& framebuffer)
{
  auto it = _framebuffer_set.find(framebuffer.get_handle());
  if (it != _framebuffer_set.end()) {
    glDeleteFramebuffers(1, &*it);
    _framebuffer_set.erase(it);
  }

  delete_texture(framebuffer.get_texture());

  it = _framebuffer_depth_set.find(framebuffer.get_depth_handle());
  if (framebuffer.get_depth_handle() && it != _framebuffer_depth_set.end()) {
    glDeleteRenderbuffers(1, &*it);
    _framebuffer_depth_set.erase(it);
  }
}

GlTexture2D GlUtil::make_texture(const y::string& filename, bool loop)
{
  y::string data;
  _filesystem.read_file(data, filename);
  if (data.empty()) {
    std::cerr << "Couldn't read file " << filename << std::endl;
    return GlTexture2D();
  }

  sf::Image image;
  if (!image.loadFromMemory(data.data(), data.length())) {
    std::cerr << "Couldn't load image " << filename << std::endl;
    return GlTexture2D();
  }
  y::ivec2 size{y::int32(image.getSize().x), y::int32(image.getSize().y)};
  GlTexture2D texture(make_texture<GLubyte>(size, GL_RGBA8, GL_RGBA,
                                            image.getPixelsPtr(), loop));

  auto it = _texture_map.find(filename);
  if (it != _texture_map.end()) {
    GLuint handle = it->second.get_handle();
    glDeleteTextures(1, &handle);
    _texture_map.erase(it);
  }
  _texture_map.insert(y::make_pair(filename, texture));
  return texture;
}

GlUnique<GlTexture2D> GlUtil::make_unique_texture(
    const y::string& filename, bool loop)
{
  return make_unique(make_texture(filename, loop));
}

GlTexture2D GlUtil::get_texture(const y::string& filename) const
{
  auto it = _texture_map.find(filename);
  return it == _texture_map.end() ? GlTexture2D(0, y::ivec2()) : it->second;
}

void GlUtil::delete_texture(const y::string& filename)
{
  auto it = _texture_map.find(filename);
  if (it != _texture_map.end()) {
    GLuint handle = it->second.get_handle();
    glDeleteTextures(1, &handle);
    auto jt = _texture_set.find(it->second.get_handle());
    if (jt != _texture_set.end()) {
      _texture_set.erase(jt);
    }
    _texture_map.erase(it);
  }
}

GlShader GlUtil::make_shader(const y::string& filename, GLenum type)
{
  static const y::string header = "#version 120\n";

  y::string data = header;
  if (!_filesystem.read_file_with_includes(data, filename)) {
    std::cerr << "Couldn't read file " << filename << std::endl;
    return GlShader();
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
  std::cerr << data << std::endl;
  GLint log_length;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
  y::unique<char[]> log(new char[log_length]);
  glGetShaderInfoLog(shader, log_length, 0, log.get());
  std::cerr << log.get();
  glDeleteShader(shader);
  return GlShader();
}

GlUnique<GlShader> GlUtil::make_unique_shader(
    const y::string& filename, GLenum type)
{
  return make_unique(make_shader(filename, type));
}

GlShader GlUtil::get_shader(const y::string& filename) const
{
  auto it = _shader_map.find(filename);
  return it == _shader_map.end() ? GlShader() : it->second;
}

void GlUtil::delete_shader(const y::string& filename)
{
  auto it = _shader_map.find(filename);
  if (it != _shader_map.end()) {
    glDeleteShader(it->second.get_handle());
    _shader_map.erase(it);
  }
}

void GlUtil::delete_shader(const GlShader& shader)
{
  for (auto it = _shader_map.begin(); it != _shader_map.end(); ++it) {
    if (it->second.get_handle() == shader.get_handle()) {
      glDeleteShader(it->second.get_handle());
      _shader_map.erase(it);
      break;
    }
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
  return GlProgram();
}

GlUnique<GlProgram> GlUtil::make_unique_program(
    const y::string_vector& shaders)
{
  return make_unique(make_program(shaders));
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
  return it == _program_map.end() ? GlProgram() : it->second;
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

void GlUtil::delete_program(const GlProgram& program)
{
  for (auto it = _program_map.begin(); it != _program_map.end(); ++it) {
    if (it->second.get_handle() == program.get_handle()) {
      glDeleteProgram(it->second.get_handle());
      _program_map.erase(it);
      break;
    }
  }
}

void GlUtil::bind_window(bool clear, bool clear_depth) const
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  const Resolution& mode = _window.get_mode();
  glViewport(0, 0, mode.size[xx], mode.size[yy]);
  glClear((GL_COLOR_BUFFER_BIT * clear) | (GL_DEPTH_BUFFER_BIT * clear_depth));
}

void GlUtil::enable_depth(bool depth, GLenum test) const
{
  if (depth) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(test);
    glDepthRange(0.f, 1.f);
  }
  else {
    glDisable(GL_DEPTH_TEST);
  }
}

void GlUtil::enable_blend(bool blend, GLenum source, GLenum target) const
{
  if (blend) {
    glEnable(GL_BLEND);
    glBlendFunc(source, target);
  }
  else {
    glDisable(GL_BLEND);
  }
}

void GlUtil::draw_arrays(GLenum mode, GLsizei count) const
{
  glDrawArrays(mode, 0, count);
}
