#ifndef GAME_STAGE_H
#define GAME_STAGE_H

#include "common.h"
#include "modal.h"
#include "world.h"

class CellMap;
class GlFramebuffer;
class RenderUtil;

class GameStage : public Modal {
public:

  GameStage(RenderUtil& util, GlFramebuffer& framebuffer,
            const CellMap& map, const y::ivec2& coord);
  virtual ~GameStage();

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  y::ivec2 world_to_camera(const y::ivec2& v) const;
  y::ivec2 camera_to_world(const y::ivec2& v) const;

  RenderUtil& _util;
  GlFramebuffer& _framebuffer;
  const CellMap& _map;

  WorldWindow _world;
  y::ivec2 _camera;

};

#endif
