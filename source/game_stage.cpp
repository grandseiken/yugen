#include "game_stage.h"

#include "databank.h"
#include "lua.h"
#include "tileset.h"
#include "render_util.h"

#include <SFML/Window.hpp>

GameStage::GameStage(const Databank& bank,
                     RenderUtil& util, GlFramebuffer& framebuffer,
                     const CellMap& map, const y::ivec2& coord)
  : _bank(bank)
  , _util(util)
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

const Databank& GameStage::get_bank() const
{
  return _bank;
}

RenderUtil& GameStage::get_util() const
{
  return _util;
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
    _camera -= {0, 2};
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    _camera -= {2, 0};
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
    _camera += {0, 2};
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
    _camera += {2, 0};
  }
}

void GameStage::draw() const
{
  // Render all the tiles in the world at once, batched by texture.
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
        // Foreground: .4
        // Collision layer: .5
        // Background: .6
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

  // Render test script.
  // TODO: testing.
  const LuaFile& file = _bank.scripts.get("/scripts/hello.lua");
  Script test_script(*const_cast<GameStage*>(this), file.path, file.contents);
  test_script.call("draw");

  // Render geometry.
  if (!sf::Keyboard::isKeyPressed(sf::Keyboard::G)) {
    return;
  }
  y::size i = 0;
  for (const Geometry& g : _world.get_geometry()) {
    Colour c = ++i % 2 ? Colour(1.f, 0.f, 0.f, .5f) :
                         Colour(0.f, 1.f, 0.f, .5f);
    _util.render_fill(
        world_to_camera(y::min(g.start, g.end)),
        y::max(y::abs(g.end - g.start), y::ivec2{1, 1}), c);
  }
}

y::ivec2 GameStage::world_to_camera(const y::ivec2& v) const
{
  return v - _camera + _framebuffer.get_size() / 2;
}

y::ivec2 GameStage::camera_to_world(const y::ivec2& v) const
{
  return v - _framebuffer.get_size() / 2 + _camera;
}
