#ifndef YUGEN_H
#define YUGEN_H

#include "modal.h"
#include "render/gl_handle.h"

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

  Yugen(RenderUtil& util, RunTiming& run_timing);
  ~Yugen() override;

  const GlFramebuffer& get_framebuffer() const;
  void set_stage(GameStage* stage);

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  void crop_render(const GlFramebuffer& source,
                   const y::ivec2& target_size) const;
  void post_render(const GlFramebuffer& source,
                   const y::ivec2& target_size) const;
  void upscale_render(const GlFramebuffer& source,
                      const y::ivec2& target_size) const;
  void recording_render(const GlFramebuffer& source) const;

  mutable bool _recording;
  mutable y::vector<unsigned char*> _save_file_frames;

  RenderUtil& _util;
  RunTiming& _run_timing;

  GlUnique<GlFramebuffer> _framebuffer;
  GlUnique<GlFramebuffer> _crop_buffer;
  GlUnique<GlFramebuffer> _post_buffer;
  GameStage* _stage;

  GlUnique<GlProgram> _crop_program;
  GlUnique<GlProgram> _post_program;
  GlUnique<GlProgram> _upscale_program;
  GlUnique<GlTexture3D> _bayer_texture;
  GlUnique<GlTexture3D> _a_dither_texture;
  mutable y::size _dither_frame;

};

#endif
