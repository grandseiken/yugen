#ifndef RENDER_GL_HANDLE_H
#define RENDER_GL_HANDLE_H

#include "../common/map.h"
#include "../common/memory.h"
#include "../common/set.h"
#include "../common/string.h"
#include "../common/utility.h"
#include "../common/vector.h"
#include "../vec.h"
#include <GL/glew.h>

#define GL_TYPE(T, V) template<> struct GlType<T> {enum {type_enum = V};}
template<typename>
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

#define GL_UNIFORM(T, F1, F2, F3, F4)\
  template<> struct GlUniform<T> {\
    static void uniform1(GLint l, T a) {F1(l, a);}\
    static void uniform2(GLint l, T a, T b) {F2(l, a, b);}\
    static void uniform3(GLint l, T a, T b, T c) {F3(l, a, b, c);}\
    static void uniform4(GLint l, T a, T b, T c, T d) {F4(l, a, b, c, d);}\
  }
template<typename>
struct GlUniform {};
GL_UNIFORM(bool, glUniform1i, glUniform2i, glUniform3i, glUniform4i);
GL_UNIFORM(GLint, glUniform1i, glUniform2i, glUniform3i, glUniform4i);
GL_UNIFORM(GLuint, glUniform1ui, glUniform2ui, glUniform3ui, glUniform4ui);
GL_UNIFORM(GLfloat, glUniform1f, glUniform2f, glUniform3f, glUniform4f);
#undef GL_UNIFORM

class GlHandle {
public:

  virtual ~GlHandle() {}

  bool operator==(const GlHandle& handle) const;
  bool operator!=(const GlHandle& handle) const;

  explicit operator bool() const;

  GLuint get_handle() const;

protected:

  friend class GlUtil;
  template<typename>
  friend class GlUnique;
  explicit GlHandle(GLuint handle);

private:

  GLuint _handle;

};

// Lightweight handle to an OpenGL buffer.
template<typename T, y::size>
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
template<y::size N>
class GlTexture : public GlHandle {
public:

  typedef y::vec<y::int32, N> ivecn;

  GlTexture();
  ~GlTexture() override {}

  void bind(GLenum target) const;

  const ivecn& get_size() const;

protected:

  friend class GlUtil;
  GlTexture(GLuint handle, const ivecn& size);

private:

  ivecn _size;

};

typedef GlTexture<1> GlTexture1D;
typedef GlTexture<2> GlTexture2D;
typedef GlTexture<3> GlTexture3D;
template<y::size>
struct GlTextureEnum {};
template<> struct GlTextureEnum<1> {
  enum {dimension_enum = GL_TEXTURE_1D};
  static void tex_image(GLenum bit_depth, GLenum format, GLenum value_type,
                        const GlTexture<1>::ivecn& size, const void* data) {
    glTexImage1D(dimension_enum, 0, bit_depth, size[xx],
                 0, format, value_type, data);
  }
};
template<> struct GlTextureEnum<2> {
  enum {dimension_enum = GL_TEXTURE_2D};
  static void tex_image(GLenum bit_depth, GLenum format, GLenum value_type,
                        const GlTexture<2>::ivecn& size, const void* data) {
    glTexImage2D(dimension_enum, 0, bit_depth, size[xx], size[yy],
                 0, format, value_type, data);
  }
};
template<> struct GlTextureEnum<3> {
  enum {dimension_enum = GL_TEXTURE_3D};
  static void tex_image(GLenum bit_depth, GLenum format, GLenum value_type,
                        const GlTexture<3>::ivecn& size, const void* data) {
    glTexImage3D(dimension_enum, 0, bit_depth, size[xx], size[yy], size[zz],
                 0, format, value_type, data);
  }
};

// Lightweight handle to an OpenGL framebuffer (and underlying texture).
class GlFramebuffer : public GlHandle {
public:

  GlFramebuffer();
  ~GlFramebuffer() override {}

  // Don't delete this texture manually!
  const GlTexture2D& get_texture() const;
  const y::ivec2& get_size() const;

  // Render to framebuffer, rather than window.
  void bind(bool clear, bool clear_depth) const;

protected:

  friend class GlUtil;
  GlFramebuffer(GLuint handle, const GlTexture2D& texture, GLuint depth);

  GLuint get_depth_handle() const;

private:

  GlTexture2D _texture;
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

// Lightweight handle to a GLSL shader program.
class GlProgram : public GlHandle {
public:

  GlProgram();
  ~GlProgram() override {}

  // Render using this shader.
  void bind() const;

  // Unbind and disable an attribute.
  void unbind_attribute(const y::string& name) const;

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
  template<y::size N>
  bool bind_uniform(const y::string& name, const GlTexture<N>& arg) const;

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
  template<y::size N>
  bool bind_uniform(y::size index, const y::string& name,
                    const GlTexture<N>& arg) const;

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

  // Store which attributes are enabled so we can disable them when no longer
  // needed.
  mutable y::set<GLint> _enabled_attribute_indices;

};

template<typename T>
class GlUnique : public y::no_copy {
public:

  typedef GlUnique<T> U;

  GlUnique()
    : _gl(y::null)
  {
  }

  GlUnique(GlUtil& gl, const T& t)
    : _gl(&gl)
    , _t(t)
  {
  }

  GlUnique(GlUnique&& u)
    : _gl(u._gl)
  {
    y::swap(_t, u._t);
  }

  // Implemented in gl_util.h.
  ~GlUnique();

  void swap(GlUnique& u)
  {
    y::swap(_t, u._t);
    y::swap(_gl, u._gl);
  }

  void swap(GlUnique&& u)
  {
    y::swap(_t, u._t);
    y::swap(_gl, u._gl);
  }

  const T& operator*() const
  {
    return _t;
  }

  /***/ T& operator*()
  {
    return _t;
  }

  const T* operator->() const
  {
    return &_t;
  }

  /***/ T* operator->()
  {
    return &_t;
  }

private:

  GlUtil* _gl;
  T _t;

};

// Convenience struct to store a buffer and its data source together.
template<typename T, y::size N>
struct GlDatabuffer {

  GlDatabuffer(GlUnique<GlBuffer<T, N>>&& buffer);
  const GlBuffer<T, N>& reupload() const;

  // The data is mutable as it's generally temporary, used for rendering, and
  // populated every frame from other sources.
  mutable y::vector<T> data;
  GlUnique<GlBuffer<T, N>> buffer;

};

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

template<y::size N>
GlTexture<N>::GlTexture()
  : GlHandle(0)
{
}

template<y::size N>
GlTexture<N>::GlTexture(GLuint handle, const ivecn& size)
  : GlHandle(handle)
  , _size(size)
{
}

template<y::size N>
void GlTexture<N>::bind(GLenum target) const
{
  glActiveTexture(target);
  glBindTexture(GlTextureEnum<N>::dimension_enum, get_handle());
}

template<y::size N>
const typename GlTexture<N>::ivecn& GlTexture<N>::get_size() const
{
  return _size;
}

template<typename T, y::size N>
bool GlProgram::bind_attribute(const y::string& name,
                               const GlBuffer<T, N>& buffer) const
{
  if (!check_match(true, name, false, 0, GlType<T>::type_enum, N)) {
    return false;
  }

  GLint location = get_attribute_location(name);
  _enabled_attribute_indices.insert(location);
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

template<y::size N>
bool GlProgram::bind_uniform(const y::string& name,
                             const GlTexture<N>& arg) const
{
  arg.bind(GL_TEXTURE0 + _texture_index);
  return bind_uniform(name, _texture_index++);
}

template<typename T, y::size N>
bool GlProgram::bind_attribute(y::size index, const y::string& name,
                               const GlBuffer<T, N>& buffer) const
{
  if (!check_match(true, name, true, index, GlType<T>::type_enum, N)) {
    return false;
  }

  GLint location = get_attribute_location(name, index);
  _enabled_attribute_indices.insert(location);
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

template<y::size N>
bool GlProgram::bind_uniform(y::size index, const y::string& name,
                             const GlTexture<N>& arg) const
{
  arg.bind(GL_TEXTURE0 + _texture_index);
  return bind_uniform(index, name, _texture_index++);
}

template<typename T, y::size N>
GlDatabuffer<T, N>::GlDatabuffer(GlUnique<GlBuffer<T, N>>&& buffer)
  : buffer(y::forward<GlUnique<GlBuffer<T, N>>>(buffer))
{
}

template<typename T, y::size N>
const GlBuffer<T, N>& GlDatabuffer<T, N>::reupload() const
{
  buffer->reupload_data(data);
  return *buffer;
}

#endif
