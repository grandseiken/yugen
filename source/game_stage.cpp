#include "game_stage.h"

#include "databank.h"
#include "gl_util.h"
#include "tileset.h"
#include "window.h"

ScriptBank::ScriptBank(GameStage& stage)
  : _stage(stage)
{
}

Script& ScriptBank::create_script(const LuaFile& file, const y::wvec2& origin)
{
  return create_script(file, origin, y::wvec2(Tileset::tile_size));
}

Script& ScriptBank::create_script(
    const LuaFile& file, const y::wvec2& origin, const y::wvec2& region)
{
  Script* s = new Script(_stage, file.path, file.contents, origin, region);
  add_script(y::move_unique(s));
  return *s;
}

void ScriptBank::add_script(y::unique<Script> script)
{
  _scripts.emplace_back();
  (_scripts.end() - 1)->swap(script);
}

void ScriptBank::update_all() const
{
  // Can't use iterators, or they will be invalidated by new creations.
  // Destructions won't happen here, though.
  for (y::size i = 0; i < _scripts.size(); ++i) {
    const auto& script = _scripts[i];
    if (script->has_function("update") && !script->is_destroyed()) {
      script->call("update");
    }
  }
}

void ScriptBank::move_all(const y::wvec2& move) const
{
  for (const auto& script : _scripts) {
    script->set_origin(script->get_origin() + move);
  }
}

void ScriptBank::render_all(const y::wvec2& camera_min,
                            const y::wvec2& camera_max) const
{
  for (const auto& script : _scripts) {
    const y::wvec2 min = script->get_origin() - script->get_region() / 2;
    const y::wvec2 max = script->get_origin() + script->get_region() / 2;
    if (max > camera_min && min < camera_max &&
        script->has_function("draw")) {
      script->call("draw");
    }
  }
}

void ScriptBank::get_unrefreshed(WorldWindow& world)
{
  // Find unrefreshed cells. In the common case where all cells are unrefreshed
  // we can optimise most of the things away.
  _all_unrefreshed = world.get_refreshed_cells().empty();
  _unrefreshed.clear();
  for (auto it = world.get_cartesian(); !_all_unrefreshed && it; ++it) {
    bool refreshed = false;
    for (const y::ivec2& cell : world.get_refreshed_cells()) {
      if (cell == *it) {
        refreshed = true;
        break;
      }
    }
    if (!refreshed) {
      _unrefreshed.emplace_back(*it);
    }
  }
  world.clear_refreshed_cells();
}

void ScriptBank::clean_out_of_bounds(
    const Script& player, const y::wvec2& lower, const y::wvec2& upper)
{
  struct local {
    bool all_unrefreshed;
    // TODO: remove.
    const WorldWindow::cell_list& unrefreshed;
    const Script* player;

    const y::wvec2& lower;
    const y::wvec2& upper;

    local(bool all_unrefreshed, const WorldWindow::cell_list& unrefreshed,
          const Script* player, const y::wvec2& lower, const y::wvec2& upper)
      : all_unrefreshed(all_unrefreshed)
      , unrefreshed(unrefreshed)
      , player(player)
      , lower(lower)
      , upper(upper)
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
        overlaps_unrefreshed = origin + region / 2 >= lower &&
                               origin - region / 2 < upper;
      }
      return !overlaps_unrefreshed;
    }
  };

  local is_out_of_bounds(_all_unrefreshed, _unrefreshed, &player, lower, upper);
  _scripts.erase(
      std::remove_if(_scripts.begin(), _scripts.end(), is_out_of_bounds),
      _scripts.end());
}

void ScriptBank::clean_destroyed()
{
  struct local {
    static bool is_destroyed(const y::unique<Script>& s)
    {
      return s->is_destroyed();
    }
  };

  _scripts.erase(
      std::remove_if(_scripts.begin(), _scripts.end(), local::is_destroyed),
      _scripts.end());
}

void ScriptBank::create_in_bounds(
    const Databank& bank, const CellMap& map, const WorldWindow& world,
    const y::wvec2& lower, const y::wvec2& upper)
{
  if (_all_unrefreshed) {
    // In this case overlaps_world == overlaps_unrefreshed, so this will never
    // do anything.
    return;
  }

  for (const ScriptBlueprint& s : map.get_scripts()) {
    WorldScript ws = world.script_blueprint_to_world_script(s);

    bool overlaps_unrefreshed = false;
    for (const y::ivec2& cell : _unrefreshed) {
      if (ws.origin + ws.region / 2 >= y::wvec2(
              cell * Tileset::tile_size * Cell::cell_size) &&
          ws.origin - ws.region / 2 < y::wvec2(
              (y::ivec2{1, 1} + cell) * Tileset::tile_size * Cell::cell_size)) {
        overlaps_unrefreshed = true;
        break;
      }
    }

    bool overlaps_world = ws.origin + ws.region / 2 >= lower &&
                          ws.origin - ws.region / 2 < upper;

    if (overlaps_world && !overlaps_unrefreshed) {
      const LuaFile& file = bank.scripts.get(ws.path);
      add_script(y::move_unique(
          new Script(_stage, file.path, file.contents, ws.origin, ws.region)));
    }
  }
}

GameStage::GameStage(const Databank& bank,
                     RenderUtil& util, const GlFramebuffer& framebuffer,
                     const CellMap& map, const y::wvec2& coord)
  : _bank(bank)
  , _util(util)
  , _map(map)
  , _framebuffer(framebuffer)
  , _colourbuffer(util.get_gl().make_unique_framebuffer(
        framebuffer.get_size(), true))
  , _normalbuffer(util.get_gl().make_unique_framebuffer(
        framebuffer.get_size(), false))
  , _lightbuffer(util.get_gl().make_unique_framebuffer(
        framebuffer.get_size(), false))
  , _scene_program(util.get_gl().make_unique_program({
        "/shaders/scene.v.glsl",
        "/shaders/scene.f.glsl"}))
  , _current_draw_stage(draw_stage(0))
  , _world(map, y::ivec2(coord + y::wvec2{.5, .5}).euclidean_div(
        Tileset::tile_size * Cell::cell_size))
  , _scripts(*this)
  , _collision(_world)
  , _lighting(_world, util.get_gl())
  , _environment(util.get_gl())
  , _camera_rotation(0)
  , _is_camera_moving_x(false)
  , _is_camera_moving_y(false)
  , _player(y::null)
{
  const LuaFile& file = _bank.scripts.get("/scripts/game/player.lua");
  y::wvec2 offset = y::wvec2(_world.get_active_coord() *
                             Cell::cell_size * Tileset::tile_size);
  Script& player = _scripts.create_script(file, coord - offset);
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
  util.get_window().set_key_repeat(false);
  _key_map[KEY_UP] = {sf::Keyboard::W, sf::Keyboard::Up,
                      sf::Keyboard::LControl, sf::Keyboard::RControl};
  _key_map[KEY_DOWN] = {sf::Keyboard::S, sf::Keyboard::Down};
  _key_map[KEY_LEFT] = {sf::Keyboard::A, sf::Keyboard::Left};
  _key_map[KEY_RIGHT] = {sf::Keyboard::D, sf::Keyboard::Right};
}

const Databank& GameStage::get_bank() const
{
  return _bank;
}

RenderUtil& GameStage::get_util() const
{
  return _util;
}

const ScriptBank& GameStage::get_scripts() const
{
  return _scripts;
}

ScriptBank& GameStage::get_scripts()
{
  return _scripts;
}

const Collision& GameStage::get_collision() const
{
  return _collision;
}

Collision& GameStage::get_collision()
{
  return _collision;
}

const Lighting& GameStage::get_lighting() const
{
  return _lighting;
}

Lighting& GameStage::get_lighting()
{
  return _lighting;
}

const Environment& GameStage::get_environment() const
{
  return _environment;
}

Environment& GameStage::get_environment()
{
  return _environment;
}

y::wvec2 GameStage::world_to_camera(const y::wvec2& v) const
{
  return v - _camera + y::wvec2(_framebuffer.get_size()) / 2;
}

y::wvec2 GameStage::camera_to_world(const y::wvec2& v) const
{
  return v - y::wvec2(_framebuffer.get_size()) / 2 + _camera;
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

  // Update scripts.
  _scripts.update_all();

  // Update window.
  const y::int32 cell_switch_buffer = 2;
  y::wvec2 origin = -y::wvec2(cell_switch_buffer * Tileset::tile_size);
  y::wvec2 size = y::wvec2(Tileset::tile_size *
                           (y::ivec2{cell_switch_buffer,
                                     cell_switch_buffer} + Cell::cell_size));
  if (get_player()) {
    y::wvec2 p_origin = get_player()->get_origin();
    if (!p_origin.in_region(origin, size)) {
      y::ivec2 move = y::ivec2(p_origin + y::wvec2{.5, .5}).euclidean_div(
                          Tileset::tile_size * Cell::cell_size);
      _world.move_active_window(move);
      y::wvec2 script_move = y::wvec2(
          move * Cell::cell_size * Tileset::tile_size);
      _scripts.move_all(-script_move);
      _camera -= script_move;
    }
  }

  // Clean up out-of-bounds scripts, add new in-bounds scripts, then clean up
  // manually-destroyed scripts.
  _scripts.get_unrefreshed(_world);
  _scripts.clean_out_of_bounds(*_player, lower_bound, upper_bound);
  _scripts.create_in_bounds(_bank, _map, _world, lower_bound, upper_bound);
  _scripts.clean_destroyed();

  // Clean up from the various ScriptMaps.
  script_maps_clean_up();

  // Update camera.
  if (get_player()) {
    update_camera(get_player());
  }

  // Recalculate lighting.
  _lighting.recalculate_traces(get_camera_min(), get_camera_max());
}

void GameStage::draw() const
{
  y::fvec2 translation = y::fvec2(world_to_camera(y::wvec2()));
  _util.add_translation(translation);

  _framebuffer.bind(true, true);

  // Loop through the draw stages.
  for (_current_draw_stage = draw_stage(0);
       _current_draw_stage < DRAW_STAGE_MAX;
       _current_draw_stage = draw_stage(1 + _current_draw_stage)) {
    // Render colour buffer or normal buffer as appropriate.
    _current_batch.clear();
    _current_draw_any = false;
    if (draw_stage_is_normal(_current_draw_stage)) {
      _normalbuffer->bind(false, true);
    }
    else {
      _colourbuffer->bind(true, true);
    }

    if (draw_stage_is_layer(_current_draw_stage, DRAW_WORLD)) {
      render_tiles();
    }
    _scripts.render_all(get_camera_min(), get_camera_max());
    _util.render_batch(_current_batch);

    // If there's anything on this layer, render scene by the lighting.
    if (draw_stage_is_normal(_current_draw_stage) && _current_draw_any) {
      _lightbuffer->bind(true, false);
      _lighting.render_lightbuffer(_util, *_normalbuffer,
                                   get_camera_min(), get_camera_max());
      _framebuffer.bind(false, false);
      render_scene(true);
    }
  }

  // Render geometry.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::G)) {
    _collision.render(_util, get_camera_min(), get_camera_max());
  }

  // Render lighting.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::L)) {
    _lighting.render_traces(_util, get_camera_min(), get_camera_max());
  }

  _util.add_translation(-translation);
}

RenderBatch& GameStage::get_current_batch() const
{
  return _current_batch;
}

GameStage::draw_stage GameStage::get_current_draw_stage() const
{
  return _current_draw_stage;
}

void GameStage::set_current_draw_any() const
{
  _current_draw_any = true;
}

bool GameStage::draw_stage_is_normal(draw_stage stage) const
{
  return stage % 2;
}

bool GameStage::draw_stage_is_layer(draw_stage stage, draw_layer layer) const
{
  return stage / 2 == layer;
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

void GameStage::set_camera(const y::wvec2& camera)
{
  _camera = camera;
}

const y::wvec2& GameStage::get_camera() const
{
  return _camera;
}

void GameStage::set_camera_rotation(y::world rotation)
{
  _camera_rotation = rotation;
}

y::world GameStage::get_camera_rotation() const
{
  return _camera_rotation;
}

void GameStage::script_maps_clean_up()
{
  _collision.clean_up();
}

void GameStage::update_camera(Script* focus)
{
  // TODO: respect rotation (i.e., use coordinate system defined by the
  // current camera rotation)?
  static const y::world camera_deadzone_size =
      y::world(y::min(RenderUtil::native_size[xx],
                      RenderUtil::native_size[yy])) / 5;
  static const y::wvec2 camera_deadzone{
      camera_deadzone_size, camera_deadzone_size};
  static const y::wvec2 camera_deadzone_buffer =
      camera_deadzone + y::wvec2{128., 128.};
  static const y::wvec2 camera_speed{2.5, 4.};

  y::wvec2 target = focus->get_origin();

  if (y::abs(target[xx] - _camera[xx]) < camera_deadzone[xx] / 2) {
    _is_camera_moving_x = false;
  }
  if (y::abs(target[yy] - _camera[yy]) < camera_deadzone[yy] / 2) {
    _is_camera_moving_y = false;
  }

  if (y::abs(target[xx] - _camera[xx]) > camera_deadzone_buffer[xx] / 2) {
    _is_camera_moving_x = true;
  }
  if (y::abs(target[yy] - _camera[yy]) > camera_deadzone_buffer[yy] / 2) {
    _is_camera_moving_y = true;
  }

  y::wvec2 dir = target - _camera;
  if (_is_camera_moving_x && dir[xx]) {
    _camera[xx] += camera_speed[xx] * (dir[xx] / y::abs(dir[xx]));
  }
  if (_is_camera_moving_y && dir[yy]) {
    _camera[yy] += camera_speed[yy] * (dir[yy] / y::abs(dir[yy]));
  }
}

y::wvec2 GameStage::get_camera_min() const
{
  return camera_to_world(y::wvec2());
}

y::wvec2 GameStage::get_camera_max() const
{
  return camera_to_world(y::wvec2(_framebuffer.get_size()));
}

void GameStage::render_tiles() const
{
  _current_draw_any = true;

  // Render all the tiles in the world at once, batched by texture.
  for (auto it = _world.get_cartesian(); it; ++it) {
    Cell* cell = _world.get_active_window_cell(*it);
    if (!cell) {
      continue;
    }
    for (auto jt = y::cartesian(Cell::cell_size); jt; ++jt) {
      y::ivec2 world = Tileset::tile_size * (*jt + *it * Cell::cell_size);
      if (y::wvec2(world + Tileset::tile_size) <= get_camera_min() ||
          y::wvec2(world) >= get_camera_max()) {
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

        const GlTexture2D texture = draw_stage_is_normal(_current_draw_stage) ?
            t.tileset->get_texture().normal : t.tileset->get_texture().texture;

        _current_batch.add_sprite(
            texture, Tileset::tile_size, y::fvec2(world),
            t.tileset->from_index(t.index), d, 0.f, colour::white);
      }
    }
  }
}

void GameStage::render_scene(bool enable_blend) const
{
  _framebuffer.bind(false, false);
  _util.get_gl().enable_depth(false);
  _util.get_gl().enable_blend(enable_blend);
  _scene_program->bind();
  _scene_program->bind_attribute("position", _util.quad_vertex());
  _scene_program->bind_uniform("colourbuffer", *_colourbuffer);
  _scene_program->bind_uniform("lightbuffer", *_lightbuffer);
  _util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}
