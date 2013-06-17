#ifndef GL_UTIL_H
#define GL_UTIL_H

#include "common.h"
#include "vector.h"

#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

class Filesystem;
class Window;

#define GL_TYPE(T, V) template<> struct GlType<T> {enum {type_enum = V};}
template<typename T>
struct GlType {};
GL_TYPE(bool, GL_BOOL);
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
  template<> struct GlUniform<T> {\
    static void uniform1(GLint l, T a) {F1(l, a);}\
    static void uniform2(GLint l, T a, T b) {F2(l, a, b);}\
    static void uniform3(GLint l, T a, T b, T c) {F3(l, a, b, c);}\
    static void uniform4(GLint l, T a, T b, T c, T d) {F4(l, a, b, c, d);}\
  }
template<typename T>
struct GlUniform {};
GL_UNIFORM(bool, glUniform1i, glUniform2i, glUniform3i, glUniform4i);
GL_UNIFORM(GLint, glUniform1i, glUniform2i, glUniform3i, glUniform4i);
GL_UNIFORM(GLuint, glUniform1ui, glUniform2ui, glUniform3ui, glUniform4ui);
GL_UNIFORM(GLfloat, glUniform1f, glUniform2f, glUniform3f, glUniform4f);
#undef GL_UNIFORM

// TODO: move GlHandle and subclasses into a separate header since they often
// need to be included when GlUtil is not needed.
// TODO: add a wrapper which automatically deletes the thing when it goes out of
// scope.
class GlHandle {
public:

  virtual ~GlHandle() {}

  bool operator==(const GlHandle& handle) const;
  bool operator!=(const GlHandle& handle) const;

  explicit operator bool() const;

  GLuint get_handle() const;

protected:

  friend class GlUtil;
  explicit GlHandle(GLuint handle);

private:

  GLuint _handle;

};

// Lightweight handle to an OpenGL buffer.
template<typename T, y::size N>
class GlBuffer : public GlHandle {
public:

  GlBuffer();
  ~GlBuffer() override {}

  void bind() const;
  void draw_elements(GLenum mode, GLsizei count) const;

  void reupload_data(const T* data, GLsizei size) const;
  void reupload_data(const y::vector<T>& data) const;

protected:

  friend class GlUtil;
  GlBuffer(GLuint handle, GLenum target, GLenum usage_hint);

private:

  GLenum _target;
  GLenum _usage_hint;

};

// Lightweight handle to an OpenGL texture.
class GlTexture : public GlHandle {
public:

  GlTexture();
  ~GlTexture() override {}

  void bind(GLenum target) const;

  const y::ivec2& get_size() const;

protected:

  friend class GlUtil;
  GlTexture(GLuint handle, const y::ivec2& size);

private:

  y::ivec2 _size;

};

// Lightweight handle to an OpenGL framebuffer (and underlying texture).
class GlFramebuffer : public GlHandle {
public:

  GlFramebuffer();
  ~GlFramebuffer() override {}

  // Don't delete this texture manually!
  const GlTexture& get_texture() const;
  const y::ivec2& get_size() const;

  // Render to framebuffer, rather than window.
  void bind(bool clear, bool clear_depth) const;

protected:

  friend class GlUtil;
  GlFramebuffer(GLuint handle, const GlTexture& texture, GLuint depth);

  GLuint get_depth_handle() const;

private:

  GlTexture _texture;
  GLuint _depth_handle;

};

class GlShader : public GlHandle {
public:

  GlShader();
  ~GlShader() override {}

protected:

  friend class GlUtil;
  explicit GlShader(GLuint handle);

};

namespace std {

  template<>
  struct hash<y::pair<y::string, y::size>> {
    y::size operator()(const y::pair<y::string, y::size>& pair) const;
  };

}

// Lightweight handle to a GLSL shader program.
class GlProgram : public GlHandle {
public:

  GlProgram();
  ~GlProgram() override {}

  // Render using this shader.
  void bind() const;

  // Bind the value of an attribute variable.
  template<typename T, y::size N>
  bool bind_attribute(const y::string& name,
                      const GlBuffer<T, N>& buffer) const;

  // Bind the value of a uniform variable.
  template<typename T>
  bool bind_uniform(const y::string& name, T a) const;
  template<typename T>
  bool bind_uniform(const y::string& name, T a, T b) const;
  template<typename T>
  bool bind_uniform(const y::string& name, T a, T b, T c) const;
  template<typename T>
  bool bind_uniform(const y::string& name, T a, T b, T c, T d) const;

  template<typename T>
  bool bind_uniform(const y::string& name, const y::vec<T, 2>& arg) const;
  template<typename T>
  bool bind_uniform(const y::string& name, const y::vec<T, 3>& arg) const;
  template<typename T>
  bool bind_uniform(const y::string& name, const y::vec<T, 4>& arg) const;

  bool bind_uniform(const y::string& name, const GlFramebuffer& arg) const;
  bool bind_uniform(const y::string& name, const GlTexture& arg) const;

  // Bind the value of an attribute variable in an array.
  template<typename T, y::size N>
  bool bind_attribute(y::size index, const y::string& name,
                      const GlBuffer<T, N>& buffer) const;

  // Bind the value of a uniform variable in an array.
  template<typename T>
  bool bind_uniform(y::size index, const y::string& name,
                    T a) const;
  template<typename T>
  bool bind_uniform(y::size index, const y::string& name,
                    T a, T b) const;
  template<typename T>
  bool bind_uniform(y::size index, const y::string& name,
                    T a, T b, T c) const;
  template<typename T>
  bool bind_uniform(y::size index, const y::string& name,
                    T a, T b, T c, T d) const;

  template<typename T>
  bool bind_uniform(y::size index, const y::string& name,
                    const y::vec<T, 2>& arg) const;
  template<typename T>
  bool bind_uniform(y::size index, const y::string& name,
                    const y::vec<T, 3>& arg) const;
  template<typename T>
  bool bind_uniform(y::size index, const y::string& name,
                    const y::vec<T, 4>& arg) const;

  bool bind_uniform(y::size index, const y::string& name,
                    const GlFramebuffer& arg) const;
  bool bind_uniform(y::size index, const y::string& name,
                    const GlTexture& arg) const;

protected:

  friend class GlUtil;
  explicit GlProgram(GLuint handle);

private:

  // Check if uniform or attribute name exists in program.
  bool check_name_exists(bool attribute, const y::string& name,
                         bool array, y::size index,
                         GLenum& type_output) const;

  // Check if name exists and has correct type and size, or print error message.
  // TODO: GL allows converting integer -> float, etc, so that should work.
  bool check_match(bool attribute, const y::string& name,
                   bool array, y::size index,
                   GLenum type, y::size length) const;

  typedef y::map<y::string, GLint> single_map;
  typedef y::map<y::pair<y::string, y::size>, GLint> array_map;

  mutable y::int32 _texture_index;

  mutable single_map _uniform_single_map;
  mutable array_map _uniform_array_map;

  mutable single_map _attribute_single_map;
  mutable array_map _attribute_array_map;

  GLint get_uniform_location(const y::string& name) const;
  GLint get_uniform_location(const y::string& name, y::size index) const;

  GLint get_attribute_location(const y::string& name) const;
  GLint get_attribute_location(const y::string& name, y::size index) const;

};

// TODO: unbind things after use? Might make errors more obvious.
// TODO: vertex attrib divisor buffers would be nice, but can't in OpenGL 2.1.
class GlUtil : public y::no_copy {
public:

  GlUtil(const Filesystem& filesystem, const Window& window);
  ~GlUtil();

  // Returns true iff everything was initialised without problems.
  explicit operator bool() const;

  const Window& get_window() const;

  // Make an OpenGL buffer.
  template<typename T, y::size N>
  GlBuffer<T, N> make_buffer(GLenum target, GLenum usage_hint);
  template<typename T, y::size N>
  GlBuffer<T, N> make_buffer(GLenum target, GLenum usage_hint,
                             const T* data, GLsizei size);
  template<typename T, y::size N>
  GlBuffer<T, N> make_buffer(GLenum target, GLenum usage_hint,
                             const y::vector<T>& data);

  // Delete an existing buffer.
  template<typename T, y::size N>
  void delete_buffer(const GlBuffer<T, N>& buffer);

  // Make an OpenGL framebuffer.
  // TODO: allow making a framebuffer with no depth component?
  GlFramebuffer make_framebuffer(const y::ivec2& size, bool has_alpha);
  // Delete an existing framebuffer.
  void delete_framebuffer(const GlFramebuffer& framebuffer);

  // Make an OpenGL texture.
  GlTexture make_texture(const y::ivec2& size, GLenum value_type,
                         GLenum bit_depth, GLenum format,
                         const void* data, bool loop = false);
  // Load texture from file.
  GlTexture make_texture(const y::string& filename, bool loop = false);
  // Get preloaded texture.
  GlTexture get_texture(const y::string& filename) const;
  // Delete preloaded texture.
  void delete_texture(const y::string& filename);
  void delete_texture(const GlTexture& texture);

  // Load shader from file. If type is 0, guesses based on extension.
  GlShader make_shader(const y::string& filename, GLenum type = 0);
  // Get preloaded shader.
  GlShader get_shader(const y::string& filename) const;
  // Delete preloaded shader.
  void delete_shader(const y::string& filename);
  void delete_shader(const GlShader& shader);

  // Link multiple shaders into one GLSL program.
  GlProgram make_program(const y::string_vector& shaders);
  // Get preloaded program.
  GlProgram get_program(const y::string_vector& shaders) const;
  // Delete preloaded program.
  void delete_program(const y::string_vector& shaders);
  void delete_program(const GlProgram& program);

  // Render to window, rather than any framebuffer.
  void bind_window(bool clear, bool clear_depth) const;

  // Enable or disable depth test.
  void enable_depth(bool depth, GLenum test = GL_LEQUAL) const;
  // Enable or disable blending.
  void enable_blend(bool blend, GLenum source = GL_SRC_ALPHA,
                                GLenum target = GL_ONE_MINUS_SRC_ALPHA) const;

private:

  bool _setup_ok;

  const Filesystem& _filesystem;
  const Window& _window;

  typedef y::set<GLuint> gl_handle_set;
  typedef y::map<y::string, GlTexture> gl_texture_map;
  typedef y::map<y::string, GlShader> gl_shader_map;
  typedef y::map<y::string, GlProgram> gl_program_map;

  gl_handle_set _buffer_set;
  gl_handle_set _texture_set;
  gl_handle_set _framebuffer_set;
  gl_handle_set _framebuffer_depth_set;

  gl_texture_map _texture_map;
  gl_shader_map _shader_map;
  gl_program_map _program_map;

};

template<typename T, y::size N>
GlBuffer<T, N> GlUtil::make_buffer(GLenum target, GLenum usage_hint)
{

  GLuint buffer;
  glGenBuffers(1, &buffer);

  _buffer_set.insert(buffer);
  return GlBuffer<T, N>(buffer, target, usage_hint);
}

template<typename T, y::size N>
GlBuffer<T, N> GlUtil::make_buffer(GLenum target, GLenum usage_hint,
                                   const T* data, GLsizei size)
{
  GlBuffer<T, N> handle = make_buffer<T, N>(target, usage_hint);
  handle.reupload_data(data, size);
  return handle;
}

template<typename T, y::size N>
GlBuffer<T, N> GlUtil::make_buffer(GLenum target, GLenum usage_hint,
                                   const y::vector<T>& data)
{
  return make_buffer<T, N>(target, usage_hint,
                           data.data(), sizeof(T) * data.size());
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
GlBuffer<T, N>::GlBuffer()
  : GlHandle(0)
  , _target(0)
  , _usage_hint(0)
{
}

template<typename T, y::size N>
GlBuffer<T, N>::GlBuffer(GLuint handle, GLenum target, GLenum usage_hint)
  : GlHandle(handle)
  , _target(target)
  , _usage_hint(usage_hint)
{
}

template<typename T, y::size N>
void GlBuffer<T, N>::bind() const
{
  glBindBuffer(_target, get_handle());
}

template<typename T, y::size N>
void GlBuffer<T, N>::draw_elements(GLenum mode, GLsizei count) const
{
  bind();
  glDrawElements(mode, N * count, GlType<T>::type_enum, (void*)0);
}

template<typename T, y::size N>
void GlBuffer<T, N>::reupload_data(const T* data, GLsizei size) const
{
  bind();
  glBufferData(_target, size, data, _usage_hint);
}

template<typename T, y::size N>
void GlBuffer<T, N>::reupload_data(const y::vector<T>& data) const
{
  reupload_data(data.data(), sizeof(T) * data.size());
}

template<typename T, y::size N>
bool GlProgram::bind_attribute(const y::string& name,
                               const GlBuffer<T, N>& buffer) const
{
  if (!check_match(true, name, false, 0, GlType<T>::type_enum, N)) {
    return false;
  }

  GLint location = get_attribute_location(name);
  glEnableVertexAttribArray(location);
  buffer.bind();
  glVertexAttribPointer(
      location, N, GlType<T>::type_enum, GL_FALSE, sizeof(T) * N, y::null);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(const y::string& name, T a) const
{
  if (!check_match(false, name, false, 0, GlType<T>::type_enum, 1)) {
    return false;
  }

  GLint location = get_uniform_location(name);
  GlUniform<T>::uniform1(location, a);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(const y::string& name, T a, T b) const
{
  if (!check_match(false, name, false, 0, GlType<T>::type_enum, 2)) {
    return false;
  }

  GLint location = get_uniform_location(name);
  GlUniform<T>::uniform2(location, a, b);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(const y::string& name, T a, T b, T c) const
{
  if (!check_match(false, name, false, 0, GlType<T>::type_enum, 3)) {
    return false;
  }

  GLint location = get_uniform_location(name);
  GlUniform<T>::uniform3(location, a, b, c);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(const y::string& name, T a, T b, T c, T d) const
{
  if (!check_match(false, name, false, 0, GlType<T>::type_enum, 4)) {
    return false;
  }

  GLint location = get_uniform_location(name);
  GlUniform<T>::uniform4(location, a, b, c, d);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(
    const y::string& name, const y::vec<T, 2>& arg) const
{
  return bind_uniform(name, arg[xx], arg[yy]);
}

template<typename T>
bool GlProgram::bind_uniform(
    const y::string& name, const y::vec<T, 3>& arg) const
{
  return bind_uniform(name, arg[xx], arg[yy], arg[zz]);
}

template<typename T>
bool GlProgram::bind_uniform(
    const y::string& name, const y::vec<T, 4>& arg) const
{
  return bind_uniform(name, arg[xx], arg[yy], arg[zz], arg[ww]);
}

template<typename T, y::size N>
bool GlProgram::bind_attribute(y::size index, const y::string& name,
                               const GlBuffer<T, N>& buffer) const
{
  if (!check_match(true, name, true, index, GlType<T>::type_enum, N)) {
    return false;
  }

  GLint location = get_attribute_location(name, index);
  glEnableVertexAttribArray(location);
  buffer.bind();
  glVertexAttribPointer(
      location, N, GlType<T>::type_enum, GL_FALSE, sizeof(T) * N, y::null);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(y::size index, const y::string& name,
                             T a) const
{
  if (!check_match(false, name, true, index, GlType<T>::type_enum, 1)) {
    return false;
  }

  GLint location = get_uniform_location(name, index);
  GlUniform<T>::uniform1(location, a);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(y::size index, const y::string& name,
                             T a, T b) const
{
  if (!check_match(false, name, true, index, GlType<T>::type_enum, 2)) {
    return false;
  }

  GLint location = get_uniform_location(name, index);
  GlUniform<T>::uniform2(location, a, b);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(y::size index, const y::string& name,
                             T a, T b, T c) const
{
  if (!check_match(false, name, true, index, GlType<T>::type_enum, 3)) {
    return false;
  }

  GLint location = get_uniform_location(name, index);
  GlUniform<T>::uniform3(location, a, b, c);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(y::size index, const y::string& name,
                             T a, T b, T c, T d) const
{
  if (!check_match(false, name, true, index, GlType<T>::type_enum, 4)) {
    return false;
  }

  GLint location = get_uniform_location(name, index);
  GlUniform<T>::uniform4(location, a, b, c, d);
  return true;
}

template<typename T>
bool GlProgram::bind_uniform(
    y::size index, const y::string& name, const y::vec<T, 2>& arg) const
{
  return bind_uniform(index, name, arg[xx], arg[yy]);
}

template<typename T>
bool GlProgram::bind_uniform(
    y::size index, const y::string& name, const y::vec<T, 3>& arg) const
{
  return bind_uniform(index, name, arg[xx], arg[yy], arg[zz]);
}

template<typename T>
bool GlProgram::bind_uniform(
    y::size index, const y::string& name, const y::vec<T, 4>& arg) const
{
  return bind_uniform(index, name, arg[xx], arg[yy], arg[zz], arg[ww]);
}

#endif
