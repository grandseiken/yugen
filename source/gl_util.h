#ifndef GL_UTIL_H
#define GL_UTIL_H

#include "common.h"

#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

class Filesystem;
class Window;

class GlUtil {
public:

  GlUtil(const Filesystem& filesystem, const Window& window,
         GLsizei width, GLsizei height);
  ~GlUtil();

  // Returns true iff everything was initialised without problems.
  bool setup_ok() const;

  // Make an OpenGL buffer.
  GLuint make_buffer(GLenum target, GLenum usage_hint,
                     const void* data, GLsizei size) const;
  // Delete an existing buffer.
  void delete_buffer(GLuint buffer);

  // Load texture from file.
  GLuint make_texture(const y::string& filename);
  // Get preloaded texture.
  GLuint get_texture(const y::string& filename) const;
  // Delete preloaded texture.
  void delete_texture(const y::string& filename);

  // Load shader from file. If type is 0, guesses based on extension.
  GLuint make_shader(const y::string& filename, GLenum type = 0);
  // Get preloaded shader.
  GLuint get_shader(const y::string& filename) const;
  // Delete preloaded shader.
  void delete_shader(const y::string& filename);

  // Link multiple shaders into one GLSL program.
  GLuint make_program(const y::string_vector& shaders);
  // Get preloaded program.
  GLuint get_program(const y::string_vector& shaders) const;
  // Delete preloaded program.
  void delete_program(const y::string_vector& shaders);

  // Render to internal framebuffer.
  void bind_framebuffer() const;
  // Get the framebuffer.
  GLuint get_framebuffer() const;

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

#endif
