#ifndef RENDER__GL_UTIL_H
#define RENDER__GL_UTIL_H

#include "gl_handle.h"
#include "../common/map.h"
#include "../common/set.h"
#include "../vector.h"
#include <GL/glew.h>

class Filesystem;
class Window;

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
  template<typename T, y::size N>
  GlUnique<GlBuffer<T, N>> make_unique_buffer(
      GLenum target, GLenum usage_hint);
  template<typename T, y::size N>
  GlUnique<GlBuffer<T, N>> make_unique_buffer(
      GLenum target, GLenum usage_hint, const T* data, GLsizei size);
  template<typename T, y::size N>
  GlUnique<GlBuffer<T, N>> make_unique_buffer(
      GLenum target, GLenum usage_hint, const y::vector<T>& data);

  // Delete an existing buffer.
  template<typename T, y::size N>
  void delete_buffer(const GlBuffer<T, N>& buffer);

  // Make an OpenGL framebuffer.
  GlFramebuffer make_framebuffer(const y::ivec2& size,
                                 bool has_alpha, bool has_depth);
  GlUnique<GlFramebuffer> make_unique_framebuffer(
      const y::ivec2& size, bool has_alpha, bool has_depth);
  // Delete an existing framebuffer.
  void delete_framebuffer(const GlFramebuffer& framebuffer);

  // Make an OpenGL texture.
  template<typename T = float, y::size N = 2>
  GlTexture<N> make_texture(const typename GlTexture<N>::ivecn& size,
                            GLenum bit_depth, GLenum format,
                            const T* data, bool loop = false);
  template<typename T = float, y::size N = 2>
  GlUnique<GlTexture<N>> make_unique_texture(
      const typename GlTexture<N>::ivecn& size,
      GLenum bit_depth, GLenum format, const T* data, bool loop = false);
  // Load texture from file.
  GlTexture2D make_texture(const y::string& filename, bool loop = false);
  GlUnique<GlTexture2D> make_unique_texture(
      const y::string& filename, bool loop = false);
  // Get preloaded texture.
  GlTexture2D get_texture(const y::string& filename) const;
  // Delete preloaded texture.
  void delete_texture(const y::string& filename);
  template<y::size N>
  void delete_texture(const GlTexture<N>& texture);

  // Load shader from file. If type is 0, guesses based on extension. Shaders
  // named in the standard way are automatically loaded, so there's no need
  // to manually call these functions.
  GlShader make_shader(const y::string& filename, GLenum type = 0);
  GlUnique<GlShader> make_unique_shader(
      const y::string& filename, GLenum type = 0);
  // Get preloaded shader.
  GlShader get_shader(const y::string& filename) const;
  // Delete preloaded shader.
  void delete_shader(const y::string& filename);
  void delete_shader(const GlShader& shader);

  // Link multiple shaders into one GLSL program.
  GlProgram make_program(const y::vector<y::string>& shaders);
  GlUnique<GlProgram> make_unique_program(const y::vector<y::string>& shaders);
  // Get preloaded program.
  GlProgram get_program(const y::vector<y::string>& shaders) const;
  // Delete preloaded program.
  void delete_program(const y::vector<y::string>& shaders);
  void delete_program(const GlProgram& program);

  // Render to window, rather than any framebuffer.
  void bind_window(bool clear, bool clear_depth) const;

  // Enable or disable depth test.
  void enable_depth(bool depth, GLenum test = GL_LEQUAL) const;
  // Enable or disable blending.
  void enable_blend(bool blend, GLenum source = GL_SRC_ALPHA,
                                GLenum target = GL_ONE_MINUS_SRC_ALPHA) const;

  // Draw arrays.
  void draw_arrays(GLenum mode, GLsizei count) const;

  template<typename T>
  GlUnique<T> make_unique(const T& t);

private:

  bool _setup_ok;

  const Filesystem& _filesystem;
  const Window& _window;

  typedef y::set<GLuint> gl_handle_set;
  typedef y::map<y::string, GlTexture2D> gl_texture_map;
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

template<typename T>
struct UniqueDelete {};
template<typename T, y::size N>
struct UniqueDelete<GlBuffer<T, N>> {
  void operator()(GlUtil& gl, const GlBuffer<T, N>& t)
  {
    gl.delete_buffer(t);
  }
};
template<>
struct UniqueDelete<GlFramebuffer> {
  void operator()(GlUtil& gl, const GlFramebuffer& t)
  {
    gl.delete_framebuffer(t);
  }
};
template<y::size N>
struct UniqueDelete<GlTexture<N>> {
  void operator()(GlUtil& gl, const GlTexture<N>& t)
  {
    gl.delete_texture(t);
  }
};
template<>
struct UniqueDelete<GlShader> {
  void operator()(GlUtil& gl, const GlShader& t)
  {
    gl.delete_shader(t);
  }
};
template<>
struct UniqueDelete<GlProgram> {
  void operator()(GlUtil& gl, const GlProgram& t)
  {
    gl.delete_program(t);
  }
};

template<typename T>
GlUnique<T>::~GlUnique()
{
  if (!_t._handle) {
    return;
  }
  UniqueDelete<T> d;
  d(*_gl, _t);
}

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
GlUnique<GlBuffer<T, N>> GlUtil::make_unique_buffer(
    GLenum target, GLenum usage_hint)
{
  return make_unique(make_buffer<T, N>(target, usage_hint));
}

template<typename T, y::size N>
GlUnique<GlBuffer<T, N>> GlUtil::make_unique_buffer(
    GLenum target, GLenum usage_hint, const T* data, GLsizei size)
{
  return make_unique(make_buffer<T, N>(target, usage_hint, data, size));
}

template<typename T, y::size N>
GlUnique<GlBuffer<T, N>> GlUtil::make_unique_buffer(
    GLenum target, GLenum usage_hint, const y::vector<T>& data)
{
  return make_unique(make_buffer<T, N>(target, usage_hint, data));
}

template<typename T, y::size N>
GlTexture<N> GlUtil::make_texture(const typename GlTexture<N>::ivecn& size,
                                  GLenum bit_depth, GLenum format,
                                  const T* data, bool loop)
{
  GLenum wrap_type = loop ? GL_REPEAT : GL_CLAMP_TO_EDGE;
  GLenum dimension = GlTextureEnum<N>::dimension_enum;

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(dimension, texture);
  glTexParameteri(dimension, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(dimension, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(dimension, GL_TEXTURE_WRAP_S, wrap_type);
  glTexParameteri(dimension, GL_TEXTURE_WRAP_T, wrap_type);

  GlTextureEnum<N>::tex_image(
      bit_depth, format, GlType<T>::type_enum, size, data);
  _texture_set.insert(texture);
  return GlTexture<N>(texture, size);
}

template<typename T, y::size N>
GlUnique<GlTexture<N>> GlUtil::make_unique_texture(
    const typename GlTexture<N>::ivecn& size,
    GLenum bit_depth, GLenum format, const T* data, bool loop)
{
  return make_unique(make_texture<T, N>(size, bit_depth, format, data, loop));
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

template<y::size N>
void GlUtil::delete_texture(const GlTexture<N>& texture)
{
  auto it = _texture_set.find(texture.get_handle());
  if (it != _texture_set.end()) {
    glDeleteTextures(1, &*it);
    _texture_set.erase(it);
  }
  // Make sure to clean up the map entry if we deleted a filename-loaded
  // texture directly.
  for (auto jt = _texture_map.begin(); jt != _texture_map.end(); ++jt) {
    if (jt->second.get_handle() == texture.get_handle()) {
      _texture_map.erase(jt);
      break;
    }
  }
}

template<typename T>
GlUnique<T> GlUtil::make_unique(const T& t)
{
  return GlUnique<T>(*this, t);
}

#endif
