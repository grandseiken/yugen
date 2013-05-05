#include "game_stage.h"

#include <SFML/Window.hpp>

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
  if (e.type == sf::Event::KeyPressed &&
      e.key.code == sf::Keyboard::Escape) {
    end();
  }
}

void GameStage::update()
{
}

void GameStage::draw() const
{
}
