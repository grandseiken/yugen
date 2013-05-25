#include "game_stage.h"

#include "databank.h"
#include "tileset.h"

GameStage::GameStage(const Databank& bank,
                     RenderUtil& util, const GlFramebuffer& framebuffer,
                     const CellMap& map, const y::wvec2& coord)
  : _bank(bank)
  , _util(util)
  , _framebuffer(framebuffer)
  , _map(map)
  , _world(map, y::ivec2(coord).euclidean_div(Tileset::tile_size *
                                              Cell::cell_size))
  , _collision(_world)
  , _player(y::null)
{
  const LuaFile& file = _bank.scripts.get("/scripts/player.lua");
  y::wvec2 offset = y::wvec2(_world.get_active_coord() *
                             Cell::cell_size * Tileset::tile_size);
  Script& player = create_script(file, coord - offset);
  set_player(&player);
  _camera = coord - offset;

  // Must be kept consistent with keys.lua.
  enum key_codes {
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
  };

  // Setup key bindings.
  _key_map[KEY_UP] = {sf::Keyboard::W};
  _key_map[KEY_DOWN] = {sf::Keyboard::S};
  _key_map[KEY_LEFT] = {sf::Keyboard::A};
  _key_map[KEY_RIGHT] = {sf::Keyboard::D};
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

const y::wvec2& GameStage::get_camera() const
{
  return _camera;
}

void GameStage::event(const sf::Event& e)
{
  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  for (const auto& pair : _key_map) {
    bool b = false;
    for (const auto& k : pair.second) {
      if (k == e.key.code) {
        b = true;
        break;
      }
    }
    if (b && get_player()->has_function("key")) {
      get_player()->call("key", pair.first);
    }
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
  static const y::int32 half_size = WorldWindow::active_window_half_size;
  static const y::wvec2& lower_bound = y::wvec2(
      -half_size * Cell::cell_size * Tileset::tile_size);
  static const y::wvec2& upper_bound = y::wvec2(
      (1 + half_size) * Cell::cell_size * Tileset::tile_size);

  // Local functions.
  struct local {
    bool all_unrefreshed;
    const WorldWindow::cell_list& unrefreshed;
    const Script* player;

    local(bool all_unrefreshed, const WorldWindow::cell_list& unrefreshed,
          const Script* player)
      : all_unrefreshed(all_unrefreshed)
      , unrefreshed(unrefreshed)
      , player(player)
    {
    }

    bool operator()(const y::unique<Script>& s)
    {
      if (s.get() == player) {
        return false;
      }
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
      unrefreshed.emplace_back(*it);
    }
  }
  _world.clear_refreshed_cells();

  // Clean up out-of-bounds scripts.
  local is_out_of_bounds(all_unrefreshed, unrefreshed, _player);
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

  // Update camera.
  if (get_player()) {
    const y::wvec2& p = get_player()->get_origin();
    _camera = p;
  }
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
    _collision.render(_util, camera_min, camera_max);
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

void GameStage::set_player(Script* player)
{
  _player = player;
}

Script* GameStage::get_player() const
{
  return _player;
}

bool GameStage::is_key_down(y::int32 key) const
{
  auto it = _key_map.find(key);
  if (it == _key_map.end()) {
    return false;
  }
  for (const auto& sfml_key : it->second) {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key(sfml_key))) {
      return true;
    }
  }
  return false;
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
  _scripts.emplace_back();
  (_scripts.end() - 1)->swap(script);
}
