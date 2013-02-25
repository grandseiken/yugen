#ifndef GL_UTIL_H
#define GL_UTIL_H

#include "common.h"

#include <boost/utility.hpp>
#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

class Filesystem;
class Window;

class GlUtil;
template<typename T, y::size N>
class GlBuffer;
class GlTexture;
class GlShader;
class GlProgram;

#define GL_TYPE(T, V) template<> struct GlType<T> { enum { type_enum = V }; }
template<typename T>
struct GlType {};
GL_TYPE(GLbyte, GL_BYTE);
GL_TYPE(GLubyte, GL_UNSIGNED_BYTE);
GL_TYPE(GLshort, GL_SHORT);
GL_TYPE(GLushort, GL_UNSIGNED_SHORT);
GL_TYPE(GLint, GL_INT);
GL_TYPE(GLuint, GL_UNSIGNED_INT);
GL_TYPE(GLfloat, GL_FLOAT);
GL_TYPE(GLdouble, GL_DOUBLE);
#undef GL_TYPE

#define GL_UNIFORM(T, F1, F2, F3, F4) \
  template<> struct GlUniform<T> { \
    static void uniform1(GLint l, T a) { F1(l, a); } \
    static void uniform2(GLint l, T a, T b) { F2(l, a, b); } \
    static void uniform3(GLint l, T a, T b, T c) { F3(l, a, b, c); } \
    static void uniform4(GLint l, T a, T b, T c, T d) { F4(l, a, b, c, d); } \
  }
template<typename T>
struct GlUniform {};
GL_UNIFORM(GLint, glUniform1i, glUniform2i, glUniform3i, glUniform4i);
GL_UNIFORM(GLuint, glUniform1ui, glUniform2ui, glUniform3ui, glUniform4ui);
GL_UNIFORM(GLfloat, glUniform1f, glUniform2f, glUniform3f, glUniform4f);
#undef GL_UNIFORM

class GlHandle {

  typedef void (GlHandle::*bool_type)() const;
  void no_bool_comparison() const {}

public:

  virtual ~GlHandle() {}

  bool operator==(const GlHandle& handle) const;
  bool operator!=(const GlHandle& handle) const;

  operator bool_type() const;

protected:

  friend class GlUtil;
  GlHandle(const GlUtil& gl_util, GLuint handle);

  const GlUtil& get_gl_util() const;
  GLuint get_handle() const;

private:

  const GlUtil* _gl_util;
  GLuint _handle;

};

class GlUtil : public boost::noncopyable {

  typedef void (GlUtil::*bool_type)() const;
  void no_bool_comparison() const {}

public:

  GlUtil(const Filesystem& filesystem, const Window& window,
         GLsizei width, GLsizei height);
  ~GlUtil();

  // Returns true iff everything was initialised without problems.
  operator bool_type() const;

  // Make an OpenGL buffer.
  template<typename T, y::size N>
  GlBuffer<T, N> make_buffer(GLenum target, GLenum usage_hint,
                             const T* data, GLsizei size);
  // Delete an existing buffer.
  template<typename T, y::size N>
  void delete_buffer(const GlBuffer<T, N>& buffer);

  // Load texture from file.
  GlTexture make_texture(const y::string& filename);
  // Get preloaded texture.
  GlTexture get_texture(const y::string& filename) const;
  // Delete preloaded texture.
  void delete_texture(const y::string& filename);

  // Load shader from file. If type is 0, guesses based on extension.
  GlShader make_shader(const y::string& filename, GLenum type = 0);
  // Get preloaded shader.
  GlShader get_shader(const y::string& filename) const;
  // Delete preloaded shader.
  void delete_shader(const y::string& filename);

  // Link multiple shaders into one GLSL program.
  GlProgram make_program(const y::string_vector& shaders);
  // Get preloaded program.
  GlProgram get_program(const y::string_vector& shaders) const;
  // Delete preloaded program.
  void delete_program(const y::string_vector& shaders);

  // Get the framebuffer.
  GlTexture get_framebuffer() const;
  // Render to internal framebuffer.
  void bind_framebuffer() const;
  // Render to window.
  void bind_window() const;

private:

  bool _setup_ok;

  const Filesystem& _filesystem;
  const Window& _window;
  
  GLuint _framebuffer;
  GLuint _framebuffer_texture;
  GLsizei _framebuffer_width;
  GLsizei _framebuffer_height;

  typedef y::set<GLuint> gl_set;
  typedef y::map<y::string, GLuint> gl_map;

  gl_set _buffer_set;
  gl_map _texture_map;
  gl_map _shader_map;
  gl_map _program_map;

};

template<typename T, y::size N>
class GlBuffer : public GlHandle {
public:

  virtual ~GlBuffer() {}

  void bind(GLenum target) const;
  void draw_elements(GLenum mode, GLsizei count) const;

protected:

  friend class GlUtil;
  GlBuffer(const GlUtil& gl_util, GLuint handle);

};

class GlTexture : public GlHandle {
public:

  virtual ~GlTexture() {}

  void bind(GLenum target) const;

protected:

  friend class GlUtil;
  GlTexture(const GlUtil& gl_util, GLuint handle);

};

class GlShader : public GlHandle {
public:

  virtual ~GlShader() {}

protected:

  friend class GlUtil;
  GlShader(const GlUtil& gl_util, GLuint handle);

};

class GlProgram : public GlHandle {
public:

  virtual ~GlProgram() {}

  void bind() const;

  template<typename T, y::size N>
  void bind_attribute(const y::string& name,
                      const GlBuffer<T, N>& buffer) const;

  template<typename T>
  void bind_uniform(const y::string& name, T a) const; 
  template<typename T>
  void bind_uniform(const y::string& name, T a, T b) const; 
  template<typename T>
  void bind_uniform(const y::string& name, T a, T b, T c) const; 
  template<typename T>
  void bind_uniform(const y::string& name, T a, T b, T c, T d) const;

protected:

  friend class GlUtil;
  GlProgram(const GlUtil& gl_util, GLuint handle);

};

template<typename T, y::size N>
GlBuffer<T, N> GlUtil::make_buffer(GLenum target, GLenum usage_hint,
                                   const T* data, GLsizei size)
{
  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(target, buffer);
  glBufferData(target, size, data, usage_hint);

  _buffer_set.insert(buffer);
  return GlBuffer<T, N>(*this, buffer);
}

template<typename T, y::size N>
void GlUtil::delete_buffer(const GlBuffer<T, N>& buffer)
{
  auto it = _buffer_set.find(buffer._handle);
  if (it != _buffer_set.end()) {
    glDeleteBuffers(1, &*it);
    _buffer_set.erase(it);
  }
}

template<typename T, y::size N>
GlBuffer<T, N>::GlBuffer(const GlUtil& gl_util, GLuint handle)
  : GlHandle(gl_util, handle)
{
}

template<typename T, y::size N>
void GlBuffer<T, N>::bind(GLenum target) const
{
  glBindBuffer(target, get_handle());
}

template<typename T, y::size N>
void GlBuffer<T, N>::draw_elements(GLenum mode, GLsizei count) const
{
  bind(GL_ELEMENT_ARRAY_BUFFER);
  glDrawElements(mode, N * count, GlType<T>::type_enum, (void*)0);
}

template<typename T, y::size N>
void GlProgram::bind_attribute(const y::string& name,
                               const GlBuffer<T, N>& buffer) const
{
  GLint location = glGetAttribLocation(get_handle(), name.c_str());
  glEnableVertexAttribArray(location);
  buffer.bind(GL_ARRAY_BUFFER);
  glVertexAttribPointer(
      location, N, GlType<T>::type_enum, GL_FALSE, sizeof(T) * N, y::null);
}

template<typename T>
void GlProgram::bind_uniform(const y::string& name, T a) const
{
  GLint location = glGetUniformLocation(get_handle(), name.c_str());
  GlUniform<T>::uniform1(location, a);
}

template<typename T>
void GlProgram::bind_uniform(const y::string& name, T a, T b) const
{
  GLint location = glGetUniformLocation(get_handle(), name.c_str());
  GlUniform<T>::uniform2(location, a, b);
}

template<typename T>
void GlProgram::bind_uniform(const y::string& name, T a, T b, T c) const
{
  GLint location = glGetUniformLocation(get_handle(), name.c_str());
  GlUniform<T>::uniform3(location, a, b, c);
}

template<typename T>
void GlProgram::bind_uniform(const y::string& name, T a, T b, T c, T d) const
{
  GLint location = glGetUniformLocation(get_handle(), name.c_str());
  GlUniform<T>::uniform4(location, a, b, c, d);
}

#endif
