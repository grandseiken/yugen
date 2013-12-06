#include "gl_handle.h"
#include "../log.h"

bool GlHandle::operator==(const GlHandle& handle) const
{
  return _handle == handle._handle;
}

bool GlHandle::operator!=(const GlHandle& handle) const
{
  return !operator==(handle);
}

GlHandle::operator bool() const
{
  return _handle;
}

GlHandle::GlHandle(GLuint handle)
  : _handle(handle)
{
}

GLuint GlHandle::get_handle() const
{
  return _handle;
}

GlFramebuffer::GlFramebuffer()
  : GlHandle(0)
  , _depth_handle(0)
{
}

GlFramebuffer::GlFramebuffer(GLuint handle, const GlTexture2D& texture,
                             GLuint depth)
  : GlHandle(handle)
  , _texture(texture)
  , _depth_handle(depth)
{
}

const GlTexture2D& GlFramebuffer::get_texture() const
{
  return _texture;
}

const y::ivec2& GlFramebuffer::get_size() const
{
  return get_texture().get_size();
}

void GlFramebuffer::bind(bool clear, bool clear_depth) const
{
  glBindFramebuffer(GL_FRAMEBUFFER, get_handle());
  glViewport(0, 0, get_texture().get_size()[xx], get_texture().get_size()[yy]);
  glClear((GL_COLOR_BUFFER_BIT * clear) | (GL_DEPTH_BUFFER_BIT * clear_depth));
}

GLuint GlFramebuffer::get_depth_handle() const
{
  return _depth_handle;
}

GlShader::GlShader()
  : GlHandle(0)
{
}

GlShader::GlShader(GLuint handle)
  : GlHandle(handle)
{
}

GlProgram::GlProgram()
  : GlHandle(0)
  , _texture_index(0)
{
}

GlProgram::GlProgram(GLuint handle)
  : GlHandle(handle)
  , _texture_index(0)
{
}

void GlProgram::bind() const
{
  glUseProgram(get_handle());
  _texture_index = 0;
  // Disable all attribute arrays by default.
  for (GLint location : _enabled_attribute_indices) {
    glDisableVertexAttribArray(location);
  }
  _enabled_attribute_indices.clear();
}

void GlProgram::unbind_attribute(const y::string& name) const
{
  GLint location = get_attribute_location(name);
  _enabled_attribute_indices.erase(location);
  glDisableVertexAttribArray(location);
}

bool GlProgram::bind_uniform(const y::string& name,
                             const GlFramebuffer& arg) const
{
  arg.get_texture().bind(GL_TEXTURE0 + _texture_index);
  return bind_uniform(name, _texture_index++);
}

bool GlProgram::bind_uniform(y::size index, const y::string& name,
                             const GlFramebuffer& arg) const
{
  arg.get_texture().bind(GL_TEXTURE0 + _texture_index);
  return bind_uniform(index, name, _texture_index++);
}

namespace {
  void composite_type_to_base_and_length(GLenum type, GLenum& type_output,
                                         y::size& length_output)
  {
    length_output = 1;
    type_output = 0;

    switch (type) {
      case GL_FLOAT_VEC4:
        length_output = y::max(length_output, y::size(4));
      case GL_FLOAT_VEC3:
        length_output = y::max(length_output, y::size(3));
      case GL_FLOAT_VEC2:
        length_output = y::max(length_output, y::size(2));
      case GL_FLOAT:
        type_output = GL_FLOAT;
        break;

      case GL_INT_VEC4:
        length_output = y::max(length_output, y::size(4));
      case GL_INT_VEC3:
        length_output = y::max(length_output, y::size(3));
      case GL_INT_VEC2:
        length_output = y::max(length_output, y::size(2));
      case GL_INT:
        type_output = GL_INT;
        break;

      case GL_UNSIGNED_INT_VEC4:
        length_output = y::max(length_output, y::size(4));
      case GL_UNSIGNED_INT_VEC3:
        length_output = y::max(length_output, y::size(3));
      case GL_UNSIGNED_INT_VEC2:
        length_output = y::max(length_output, y::size(2));
      case GL_UNSIGNED_INT:
        type_output = GL_UNSIGNED_INT;
        break;

      case GL_BOOL_VEC4:
        length_output = y::max(length_output, y::size(4));
      case GL_BOOL_VEC3:
        length_output = y::max(length_output, y::size(3));
      case GL_BOOL_VEC2:
        length_output = y::max(length_output, y::size(2));
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
}

bool GlProgram::check_match(bool attribute, const y::string& name,
                            bool array, y::size index,
                            GLenum type, y::size length) const
{
#ifndef DEBUG
  (void)attribute;
  (void)name;
  (void)array;
  (void)index;
  (void)type;
  (void)length;
  return true;
#else
  GLenum name_type;
  if (!check_name_exists(attribute, name, array, index, name_type)) {
    logg_err("Undefined ", attribute ? "attribute" : "uniform", " ", name);
    if (array) {
      logg_err("[", index, "]");
    }
    log_err();
    return false;
  }

  GLenum name_base_type;
  y::size name_length;
  composite_type_to_base_and_length(name_type, name_base_type, name_length);

  if (type != name_base_type) {
    log_err(attribute ? "Attribute" : "Uniform",
            " ", name, " given incorrect type");
    return false;
  }
  if (length != name_length) {
    log_err(attribute ? "Attribute" : "Uniform",
            " ", name, " given incorrect length");
    return false;
  }
  return true;
#endif
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

