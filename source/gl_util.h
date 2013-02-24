#ifndef GL_UTIL_H
#define GL_UTIL_H

#include "common.h"

#include <GL/glew.h>
#include <SFML/OpenGL.hpp>

class Filesystem;

class GlUtil {
public:

  GlUtil(const Filesystem& filesystem);
  ~GlUtil();

  bool setup_ok() const;

  GLuint make_buffer(GLenum target, GLenum usage_hint,
                     const void* data, GLsizei size) const;

  GLuint make_texture(const y::string& filename) const;

  GLuint make_shader(const y::string& filename, GLenum type) const;

  GLuint make_program(const y::vector<GLuint>& shaders) const;

private:

  bool _setup_ok;

  const Filesystem& _filesystem;

};

#endif
