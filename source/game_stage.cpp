#include "game_stage.h"

GameStage::GameStage(RenderUtil& util, GlFramebuffer& framebuffer,
                     const CellMap& map)
  : _util(util)
  , _framebuffer(framebuffer)
  , _map(map)
  , _world(map, y::ivec2())
{
}

GameStage::~GameStage()
{
}

void GameStage::event(const sf::Event& e)
{
  (void)e;
}

void GameStage::update()
{
}

void GameStage::draw() const
{
}
