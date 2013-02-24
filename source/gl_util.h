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

  // Load texture from file.
  GLuint make_texture(const y::string& filename) const;

  // Load shader from file. If type is 0, guesses based on extension.
  GLuint make_shader(const y::string& filename, GLenum type = 0) const;

  // Link multiple shaders into one GLSL program.
  GLuint make_program(const y::vector<GLuint>& shaders) const;

  // Render to internal framebuffer.
  void bind_framebuffer() const;

  // Render to window.
  void bind_window() const;
  

private:

  bool _setup_ok;

  const Filesystem& _filesystem;
  const Window& _window;
  
  GLuint _framebuffer;
  GLsizei _framebuffer_width;
  GLsizei _framebuffer_height;

};

#endif
