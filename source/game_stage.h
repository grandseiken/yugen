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

  GameStage(RenderUtil& util, GlFramebuffer& framebuffer, const CellMap& map);
  virtual ~GameStage();

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  RenderUtil& _util;
  GlFramebuffer& _framebuffer;
  const CellMap& _map;
  WorldWindow _world;

};

#endif
