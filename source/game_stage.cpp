#include "game_stage.h"

#include "tileset.h"
#include "render_util.h"

#include <SFML/Window.hpp>

GameStage::GameStage(RenderUtil& util, GlFramebuffer& framebuffer,
                     const CellMap& map, const y::ivec2& coord)
  : _util(util)
  , _framebuffer(framebuffer)
  , _map(map)
  , _world(map, coord)
  , _camera(Cell::cell_size * Tileset::tile_size * coord +
            Cell::cell_size * Tileset::tile_size / 2)
{
}

GameStage::~GameStage()
{
}

void GameStage::event(const sf::Event& e)
{
  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  switch (e.key.code) {
    case sf::Keyboard::Escape:
      end();
      break;
    default: {}
  }
}

void GameStage::update()
{
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
    _camera -= {0, 1};
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    _camera -= {1, 0};
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
    _camera += {0, 1};
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
    _camera += {1, 0};
  }
}

void GameStage::draw() const
{
  RenderBatch batch;
  for (auto it = _world.get_cartesian(); it; ++it) {
    Cell* cell = _world.get_active_window_cell(*it);
    if (!cell) {
      continue;
    }
    for (auto jt = y::cartesian(Cell::cell_size); jt; ++jt) {
      for (y::int32 layer = -Cell::background_layers;
           layer <= Cell::foreground_layers; ++layer) {
        const Tile& t = cell->get_tile(layer, *jt);
        if (!t.tileset) {
          continue;
        }
        float d = .5f - layer * .1f;

        y::ivec2 camera = world_to_camera(
            Tileset::tile_size * (*jt + *it * Cell::cell_size));
        batch.add_sprite(
            t.tileset->get_texture(), Tileset::tile_size,
            camera, t.tileset->from_index(t.index), d, Colour::white);
      }
    }
  }
  _util.render_batch(batch);
}

y::ivec2 GameStage::world_to_camera(const y::ivec2& v) const
{
  return v - _camera + _framebuffer.get_size() / 2;
}

y::ivec2 GameStage::camera_to_world(const y::ivec2& v) const
{
  return v - _framebuffer.get_size() / 2 + _camera;
}
