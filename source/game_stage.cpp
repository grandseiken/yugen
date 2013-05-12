#include "game_stage.h"

#include "databank.h"
#include "tileset.h"

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

  // Find unrefreshed cells. In the common case where all cells are unrefreshed
  // don't bother.
  WorldWindow::cell_list unrefreshed;
  bool all_unrefreshed = _world.get_refreshed_cells().empty();
  for (auto it = _world.get_cartesian(); !all_unrefreshed && it; ++it) {
    bool refreshed = false;
    for (const y::ivec2& cell : _world.get_refreshed_cells()) {
      if (cell == *it) {
        refreshed = true;
        break;
      }
    }
    if (!refreshed) {
      unrefreshed.push_back(*it);
    }
  }
  _world.clear_refreshed_cells();

  static const y::int32 half_size = WorldWindow::active_window_half_size;
  static const y::ivec2& lower_bound =
      -half_size * Cell::cell_size * Tileset::tile_size;
  static const y::ivec2& upper_bound =
      (1 + half_size) * Cell::cell_size * Tileset::tile_size;

  // Clean up out-of-bounds scripts.
  for (auto it = _scripts.begin(); it != _scripts.end();) {
    const y::ivec2& origin = (*it)->get_origin();
    const y::ivec2& region = (*it)->get_region();

    bool overlaps_unrefreshed = false;
    for (const y::ivec2& cell : unrefreshed) {
      if (origin + region / 2 >=
              cell * Tileset::tile_size * Cell::cell_size &&
          origin - region / 2 <
              (y::ivec2{1, 1} + cell) * Tileset::tile_size * Cell::cell_size) {
        overlaps_unrefreshed = true;
        break;
      }
    }
    if (all_unrefreshed) {
      overlaps_unrefreshed = origin + region / 2 >= lower_bound &&
                             origin - region / 2 < upper_bound;
    }
    it = !overlaps_unrefreshed ? _scripts.erase(it) : it + 1;
  }

  // Add new in-bounds scripts.
  for (const ScriptBlueprint& s : _map.get_scripts()) {
    if (all_unrefreshed) {
      // In this case overlaps_world == overlaps_unrefreshed, so this will never
      // do anything.
      continue;
    }
    WorldScript ws = _world.script_blueprint_to_world_script(s);

    bool overlaps_unrefreshed = false;
    for (const y::ivec2& cell : unrefreshed) {
      if (ws.origin + ws.region / 2 >=
              cell * Tileset::tile_size * Cell::cell_size &&
          ws.origin - ws.region / 2 <
              (y::ivec2{1, 1} + cell) * Tileset::tile_size * Cell::cell_size) {
        overlaps_unrefreshed = true;
        break;
      }
    }

    bool overlaps_world = ws.origin + ws.region / 2 >= lower_bound &&
                          ws.origin - ws.region / 2 < upper_bound;
    
    if (overlaps_world && !overlaps_unrefreshed) { 
      const LuaFile& file = _bank.scripts.get(ws.path);
      add_script(y::move_unique(
          new Script(*this, file.path, file.contents, ws.origin, ws.region)));
    }
  }

  // Update scripts.
  for (const auto& script : _scripts) {
    if (script->has_function("update")) {
      script->call("update");
    }
  }
}

void GameStage::draw() const
{
  _util.add_translation(world_to_camera(y::ivec2()));
  _current_batch.clear();

  // Render all the tiles in the world at once, batched by texture.
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

        y::ivec2 world = Tileset::tile_size * (*jt + *it * Cell::cell_size);
        _current_batch.add_sprite(
            t.tileset->get_texture(), Tileset::tile_size,
            world, t.tileset->from_index(t.index), d, Colour::white);
      }
    }
  }

  // Render all scripts.
  for (const auto& script : _scripts) {
    if (script->has_function("draw")) {
      script->call("draw");
    }
  }
  _util.render_batch(_current_batch);

  // Render geometry.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::G)) {
    y::size i = 0;
    for (const Geometry& g : _world.get_geometry()) {
      Colour c = ++i % 2 ? Colour(1.f, 0.f, 0.f, .5f) :
                           Colour(0.f, 1.f, 0.f, .5f);
      _util.render_fill(y::min(g.start, g.end),
                        y::max(y::abs(g.end - g.start), y::ivec2{1, 1}), c);
    }
  }
  _util.add_translation(-world_to_camera(y::ivec2()));
}

RenderBatch& GameStage::get_current_batch()
{
  return _current_batch;
}

y::ivec2 GameStage::world_to_camera(const y::ivec2& v) const
{
  return v - _camera + _framebuffer.get_size() / 2;
}

y::ivec2 GameStage::camera_to_world(const y::ivec2& v) const
{
  return v - _framebuffer.get_size() / 2 + _camera;
}

void GameStage::add_script(y::unique<Script> script)
{
  _scripts.push_back(y::unique<Script>());
  (_scripts.end() - 1)->swap(script);
}
