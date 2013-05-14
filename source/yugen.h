#ifndef YUGEN_H
#define YUGEN_H

#include "common.h"
#include "gl_util.h"
#include "modal.h"

#include <SFML/System.hpp>

class Databank;
class RenderUtil;
class Window;
namespace sf {
  class Event;
}

class Yugen : public Modal {
public:

  Yugen(Databank& bank, RenderUtil& util);
  virtual ~Yugen();

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  static const y::size samples = 16;
  mutable sf::Clock _clock;
  mutable y::vector<y::size> _measurements;
  mutable y::vector<unsigned char*> _save_file_frames;

  Databank& _bank;
  RenderUtil& _util;
  GlFramebuffer _framebuffer;
  GlFramebuffer _post_buffer;
  bool _launched;

  GlProgram _post_program;
  GlProgram _upscale_program;
  GlTexture _bayer_texture;
  mutable y::size _bayer_frame;
  GlBuffer<GLfloat, 2> _vertex_buffer;

};

#endif
