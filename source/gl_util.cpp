#include "gl_util.h"
#include "filesystem.h"
#include "window.h"

#include <boost/functional/hash.hpp>
#include <SFML/Graphics.hpp>

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

GlFramebuffer::GlFramebuffer(GLuint handle, const GlTexture& texture,
                             GLuint depth)
  : GlHandle(handle)
  , _texture(texture)
  , _depth_handle(depth)
{
}

const GlTexture& GlFramebuffer::get_texture() const
{
  return _texture;
}

y::size GlFramebuffer::get_width() const
{
  return get_texture().get_width();
}

y::size GlFramebuffer::get_height() const
{
  return get_texture().get_height();
}

void GlFramebuffer::bind(bool clear) const
{
  glBindFramebuffer(GL_FRAMEBUFFER, get_handle());
  glViewport(0, 0, get_texture().get_width(), get_texture().get_height());
  if (clear) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
}

GLuint GlFramebuffer::get_depth_handle() const
{
  return _depth_handle;
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

namespace std {

  y::size hash<y::pair<y::string, y::size>>::operator()(
      const y::pair<y::string, y::size>& pair) const
  {
    y::size seed = 0;
    boost::hash_combine(seed, pair.first);
    boost::hash_combine(seed, pair.second);
    return seed;
  }

}

GLint GlProgram::get_uniform_location(const y::string& name) const
{
  auto it = _uniform_single_map.find(name);
  if (it != _uniform_single_map.end()) {
    return it->second;
  }
  GLint location = glGetUniformLocation(get_handle(), name.c_str());
  _uniform_single_map.insert(y::make_pair(name, location));
  return location;
}

GLint GlProgram::get_uniform_location(const y::string& name,
                                      y::size index) const
{
  auto p = y::make_pair(name, index);
  auto it = _uniform_array_map.find(p);
  if (it != _uniform_array_map.end()) {
    return it->second;
  }
  y::sstream n;
  n << name << "[" << index << "]";
  GLint location = glGetUniformLocation(get_handle(), n.str().c_str());
  _uniform_array_map.insert(y::make_pair(p, location));
  return location;
}

GLint GlProgram::get_attribute_location(const y::string& name) const
{
  auto it = _attribute_single_map.find(name);
  if (it != _attribute_single_map.end()) {
    return it->second;
  }
  GLint location = glGetAttribLocation(get_handle(), name.c_str());
  _attribute_single_map.insert(y::make_pair(name, location));
  return location;
}

GLint GlProgram::get_attribute_location(const y::string& name,
                                        y::size index) const
{
  auto p = y::make_pair(name, index);
  auto it = _attribute_array_map.find(p);
  if (it != _attribute_array_map.end()) {
    return it->second;
  }
  y::sstream n;
  n << name << "[" << index << "]";
  GLint location = glGetAttribLocation(get_handle(), n.str().c_str());
  _attribute_array_map.insert(y::make_pair(p, location));
  return location;
}

void composite_type_to_base_and_length(GLenum type, GLenum& type_output,
                                       y::size& length_output)
{
  length_output = 1;
  type_output = 0;

  switch (type) {
    case GL_FLOAT_VEC4:
      length_output = std::max(length_output, y::size(4));
    case GL_FLOAT_VEC3:
      length_output = std::max(length_output, y::size(3));
    case GL_FLOAT_VEC2:
      length_output = std::max(length_output, y::size(2));
    case GL_FLOAT:
      type_output = GL_FLOAT;
      break;

    case GL_INT_VEC4:
      length_output = std::max(length_output, y::size(4));
    case GL_INT_VEC3:
      length_output = std::max(length_output, y::size(3));
    case GL_INT_VEC2:
      length_output = std::max(length_output, y::size(2));
    case GL_INT:
      type_output = GL_INT;
      break;

    case GL_UNSIGNED_INT_VEC4:
      length_output = std::max(length_output, y::size(4));
    case GL_UNSIGNED_INT_VEC3:
      length_output = std::max(length_output, y::size(3));
    case GL_UNSIGNED_INT_VEC2:
      length_output = std::max(length_output, y::size(2));
    case GL_UNSIGNED_INT:
      type_output = GL_UNSIGNED_INT;
      break;

    case GL_BOOL_VEC4:
      length_output = std::max(length_output, y::size(4));
    case GL_BOOL_VEC3:
      length_output = std::max(length_output, y::size(3));
    case GL_BOOL_VEC2:
      length_output = std::max(length_output, y::size(2));
    case GL_BOOL:
      type_output = GL_BOOL;
      break;

    case GL_DOUBLE:
      type_output = GL_DOUBLE;
      break;

    default:
      type_output = GL_INT;
  }
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

  GLenum name_base_type;
  y::size name_length;
  composite_type_to_base_and_length(name_type, name_base_type, name_length);

  if (type != name_base_type) {
    std::cerr << (attribute ? "Attribute" : "Uniform") <<
        " " << name << " given incorrect type" << std::endl;
    return false;
  }
  if (length != name_length) {
    std::cerr << (attribute ? "Attribute" : "Uniform") <<
        " " << name << " given incorrect length" << std::endl;
    return false;
  }
  return true;
}

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

  if (!GLEW_VERSION_2_1) {
    std::cerr << "OpenGL 2.1 not available" << std::endl;
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

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

GlUtil::~GlUtil()
{
  for (GLuint buffer : _buffer_set) {
    glDeleteBuffers(1, &buffer);
  }
  for (GLuint framebuffer : _framebuffer_set) {
    glDeleteFramebuffers(1, &framebuffer);
  }
  for (GLuint texture : _framebuffer_texture_set) {
    glDeleteTextures(1, &texture);
  }
  for (GLuint depth : _framebuffer_depth_set) {
    glDeleteRenderbuffers(1, &depth);
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

const Window& GlUtil::get_window() const
{
  return _window;
}

GlFramebuffer GlUtil::make_framebuffer(y::size width, y::size height)
{
  GLuint framebuffer;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
               0, GL_RGB, GL_UNSIGNED_BYTE, 0);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, texture, 0);
  GLenum draw_buffers = GL_COLOR_ATTACHMENT0;
  glDrawBuffers(1, &draw_buffers);

  GLuint depth;
  glGenRenderbuffers(1, &depth);
  glBindRenderbuffer(GL_RENDERBUFFER, depth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depth);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer isn't complete" << std::endl;
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &depth);
    return GlFramebuffer(0, GlTexture(0, 0, 0), 0);
  }

  _framebuffer_set.insert(framebuffer);
  _framebuffer_texture_set.insert(texture);
  _framebuffer_depth_set.insert(depth);
  return GlFramebuffer(framebuffer, GlTexture(texture, width, height), depth);
}

void GlUtil::delete_framebuffer(const GlFramebuffer& framebuffer)
{
  auto it = _framebuffer_set.find(framebuffer.get_handle());
  if (it != _framebuffer_set.end()) {
    glDeleteFramebuffers(1, &*it);
    _framebuffer_set.erase(it);
  }

  it = _framebuffer_texture_set.find(
      framebuffer.get_texture().get_handle());
  if (it != _framebuffer_texture_set.end()) {
    glDeleteTextures(1, &*it);
    _framebuffer_texture_set.erase(it);
  }

  it = _framebuffer_depth_set.find(framebuffer.get_depth_handle());
  if (it != _framebuffer_depth_set.end()) {
    glDeleteRenderbuffers(1, &*it);
    _framebuffer_depth_set.erase(it);
  }
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
  static const y::string header = "#version 120\n";
  static const y::string include_directive = "#include \"";
  static const y::size include_limit = 128;

  y::string data = header;
  if (!_filesystem.read_file(data, filename)) {
    std::cerr << "Couldn't read file " << filename << std::endl;
    return GlShader(0);
  }

  y::size include_count = 0;
  y::size include = data.find(include_directive);
  while (include != y::string::npos) {
    if (++include_count > include_limit) {
      std::cerr << "Include depth too deep in file " << filename << std::endl;
      return GlShader(0);
    }
    y::size begin = include + include_directive.length();
    y::size end = data.find_first_of('"', begin);
    if (end == y::string::npos) {
      break;
    }

    y::string include_filename = data.substr(begin, end - begin);
    if (include_filename[0] != '/') {
      y::string dirname;
      _filesystem.dirname(dirname, filename);
      include_filename = dirname + '/' + include_filename;
    }

    y::string after = data.substr(1 + end);
    data = data.substr(0, include);
    if (!_filesystem.read_file(data, include_filename)) {
      std::cerr << "Couldn't read file " << include_filename << std::endl;
      return GlShader(0);
    }
    data += after;
    include = data.find(include_directive, include);
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

void GlUtil::bind_window(bool clear) const
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  const Resolution& mode = _window.get_mode();
  glViewport(0, 0, mode.width, mode.height);
  if (clear) {
    glClear(GL_COLOR_BUFFER_BIT);
  }
}
