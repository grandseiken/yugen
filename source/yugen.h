#ifndef YUGEN_H
#define YUGEN_H

#include "common.h"
#include "gl_util.h"
#include "modal.h"

#include <SFML/System.hpp>

class Databank;
class GameStage;
class RenderUtil;
class Window;
namespace sf {
  class Event;
}

class Yugen : public Modal {
public:

  Yugen(RenderUtil& util);
  ~Yugen() override;

  const GlFramebuffer& get_framebuffer() const;
  void set_stage(GameStage* stage);

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  static const y::size samples = 16;
  mutable sf::Clock _clock;
  mutable y::vector<y::size> _measurements;
  mutable bool _recording;
  mutable y::vector<unsigned char*> _save_file_frames;
  mutable float _rotation;

  RenderUtil& _util;
  GlFramebuffer _framebuffer;
  GlFramebuffer _post_buffer;
  GlFramebuffer _crop_buffer;
  GameStage* _stage;

  GlProgram _post_program;
  GlProgram _crop_program;
  GlProgram _upscale_program;
  GlTexture _bayer_texture;
  mutable y::size _bayer_frame;
  GlBuffer<GLfloat, 2> _vertex_buffer;

};

#endif
