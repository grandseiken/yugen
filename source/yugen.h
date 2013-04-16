#ifndef YUGEN_H
#define YUGEN_H

#include "common.h"
#include "gl_util.h"
#include "modal.h"

#include <SFML/System.hpp>

class RenderUtil;
class Window;
namespace sf {
  class Event;
}

class Yugen : public Modal, public y::no_copy {
public:

  Yugen(Window& window, GlUtil& gl, RenderUtil& util,
        GlFramebuffer& framebuffer);
  virtual ~Yugen() {}

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  static const y::size samples = 16;
  sf::Clock _clock;
  y::vector<y::size> _measurements;

  Window& _window;
  GlUtil& _gl;
  RenderUtil& _util;
  GlFramebuffer& _framebuffer;

  bool _direction;
  GLfloat _fade_factor;

  GlProgram _hello_program;
  GlProgram _post_program;
  GlTexture _textures[3];
  GlBuffer<GLfloat, 2> _vertex_buffer;

};


#endif
