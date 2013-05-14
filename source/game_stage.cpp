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
  , _collision(_world)
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

const Collision& GameStage::get_collision() const
{
  return _collision;
}

Collision& GameStage::get_collision()
{
  return _collision;
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
    _camera -= {0., 2.};
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    _camera -= {2., 0.};
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
    _camera += {0., 2.};
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
    _camera += {2., 0.};
  }

  static const y::int32 half_size = WorldWindow::active_window_half_size;
  static const y::wvec2& lower_bound = y::wvec2(
      -half_size * Cell::cell_size * Tileset::tile_size);
  static const y::wvec2& upper_bound = y::wvec2(
      (1 + half_size) * Cell::cell_size * Tileset::tile_size);

  // Local functions.
  struct local {
    bool all_unrefreshed;
    const WorldWindow::cell_list& unrefreshed;

    local(bool all_unrefreshed, const WorldWindow::cell_list& unrefreshed)
      : all_unrefreshed(all_unrefreshed)
      , unrefreshed(unrefreshed)
    {
    }

    bool operator()(const y::unique<Script>& s)
    {
      const y::wvec2& origin = s->get_origin();
      const y::wvec2& region = s->get_region();

      bool overlaps_unrefreshed = false;
      for (const y::ivec2& cell : unrefreshed) {
        if (origin + region / 2 >= y::wvec2(
                cell * Tileset::tile_size * Cell::cell_size) &&
            origin - region / 2 < y::wvec2(
                (y::ivec2{1, 1} + cell) *
                    Tileset::tile_size * Cell::cell_size)) {
          overlaps_unrefreshed = true;
          break;
        }
      }
      if (all_unrefreshed) {
        overlaps_unrefreshed = origin + region / 2 >= lower_bound &&
                               origin - region / 2 < upper_bound;
      }
      return !overlaps_unrefreshed;
    }

    static bool is_destroyed(const y::unique<Script>& s)
    {
      return s->is_destroyed();
    }
  };

  // Find unrefreshed cells. In the common case where all cells are unrefreshed
  // we can optimise most of the things away.
  bool all_unrefreshed = _world.get_refreshed_cells().empty();
  WorldWindow::cell_list unrefreshed;
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

  // Clean up out-of-bounds scripts.
  local is_out_of_bounds(all_unrefreshed, unrefreshed);
  _scripts.erase(
      std::remove_if(_scripts.begin(), _scripts.end(), is_out_of_bounds),
      _scripts.end());

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
      if (ws.origin + ws.region / 2 >= y::wvec2(
              cell * Tileset::tile_size * Cell::cell_size) &&
          ws.origin - ws.region / 2 < y::wvec2(
              (y::ivec2{1, 1} + cell) * Tileset::tile_size * Cell::cell_size)) {
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

  // Update scripts. Can't use iterators, or they will be invalidated by new
  // creations. Destructions won't happen here, though.
  for (y::size i = 0; i < _scripts.size(); ++i) {
    const auto& script = _scripts[i];
    if (script->has_function("update") && !script->is_destroyed()) {
      script->call("update");
    }
  }

  // Clean up manually-destroyed scripts.
  _scripts.erase(
      std::remove_if(_scripts.begin(), _scripts.end(), local::is_destroyed),
      _scripts.end());
  _collision.clean_up();
}

void GameStage::draw() const
{
  y::ivec2 translation = y::ivec2(
      y::wvec2{.5, .5} + world_to_camera(y::wvec2()));
  _util.add_translation(translation);
  _current_batch.clear();
  const y::wvec2 camera_min = camera_to_world(y::wvec2());
  const y::wvec2 camera_max = 
      camera_to_world(y::wvec2(_framebuffer.get_size()));

  // Render all the tiles in the world at once, batched by texture.
  for (auto it = _world.get_cartesian(); it; ++it) {
    Cell* cell = _world.get_active_window_cell(*it);
    if (!cell) {
      continue;
    }
    for (auto jt = y::cartesian(Cell::cell_size); jt; ++jt) {
      y::ivec2 world = Tileset::tile_size * (*jt + *it * Cell::cell_size);
      if (y::wvec2(world + Tileset::tile_size) <= camera_min ||
          y::wvec2(world) >= camera_max) {
        continue;
      }

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

        _current_batch.add_sprite(
            t.tileset->get_texture(), Tileset::tile_size,
            world, t.tileset->from_index(t.index), d, colour::white);
      }
    }
  }

  // Render all scripts which overlap the screen.
  for (const auto& script : _scripts) {
    const y::wvec2 min = script->get_origin() - script->get_region() / 2;
    const y::wvec2 max = script->get_origin() + script->get_region() / 2;
    if (max > camera_min && min < camera_max && script->has_function("draw")) {
      script->call("draw");
    }
  }
  _util.render_batch(_current_batch);

  // Render geometry.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::G)) {
    y::size i = 0;
    for (const Geometry& g : _world.get_geometry()) {
      y::fvec4 c = ++i % 2 ? y::fvec4(1.f, 0.f, 0.f, .5f) :
                             y::fvec4(0.f, 1.f, 0.f, .5f);
      const y::ivec2 origin = y::min(g.start, g.end);
      const y::ivec2 size = y::max(y::abs(g.end - g.start), y::ivec2{1, 1});
      if (y::wvec2(origin + size) > camera_min &&
          y::wvec2(origin) < camera_max) {
        _util.render_fill(origin, size, c);
      }
    }
  }
  _util.add_translation(-translation);
}

Script& GameStage::create_script(const LuaFile& file, const y::wvec2& origin)
{
  return create_script(file, origin, y::wvec2(Tileset::tile_size));
}

Script& GameStage::create_script(const LuaFile& file,
                                 const y::wvec2& origin, const y::wvec2& region)
{
  Script* s = new Script(*this, file.path, file.contents, origin, region);
  add_script(y::move_unique(s));
  return *s;
}

RenderBatch& GameStage::get_current_batch()
{
  return _current_batch;
}

y::wvec2 GameStage::world_to_camera(const y::wvec2& v) const
{
  return v - _camera + y::wvec2(_framebuffer.get_size()) / 2;
}

y::wvec2 GameStage::camera_to_world(const y::wvec2& v) const
{
  return v - y::wvec2(_framebuffer.get_size()) / 2 + _camera;
}

void GameStage::add_script(y::unique<Script> script)
{
  _scripts.push_back(y::unique<Script>());
  (_scripts.end() - 1)->swap(script);
}
