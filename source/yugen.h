#ifndef YUGEN_H
#define YUGEN_H

#include "common.h"
#include "gl_handle.h"
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

  void post_render(const GlFramebuffer& source) const;
  void crop_render(const GlFramebuffer& source,
                   const y::ivec2& target_size) const;
  void upscale_render(const GlFramebuffer& source,
                      const y::ivec2& target_size) const;
  void recording_render(const GlFramebuffer& source) const;

  static const y::size samples = 16;
  mutable sf::Clock _clock;
  mutable y::vector<y::size> _measurements;
  mutable bool _recording;
  mutable y::vector<unsigned char*> _save_file_frames;

  RenderUtil& _util;
  GlUnique<GlFramebuffer> _framebuffer;
  GlUnique<GlFramebuffer> _post_buffer;
  GlUnique<GlFramebuffer> _crop_buffer;
  GameStage* _stage;

  GlUnique<GlProgram> _post_program;
  GlUnique<GlProgram> _crop_program;
  GlUnique<GlProgram> _upscale_program;
  GlUnique<GlTexture2D> _bayer_texture;
  mutable y::size _bayer_frame;

};

#endif
