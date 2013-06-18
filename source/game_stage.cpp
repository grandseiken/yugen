#include "game_stage.h"

#include "databank.h"
#include "tileset.h"
#include "window.h"

GameStage::GameStage(const Databank& bank,
                     RenderUtil& util, const GlFramebuffer& framebuffer,
                     const CellMap& map, const y::wvec2& coord)
  : _bank(bank)
  , _util(util)
  , _map(map)
  , _framebuffer(framebuffer)
  , _colourbuffer(util.get_gl().make_framebuffer(framebuffer.get_size(), true))
  , _normalbuffer(util.get_gl().make_framebuffer(framebuffer.get_size(), false))
  , _lightbuffer(util.get_gl().make_framebuffer(framebuffer.get_size(), false))
  , _scene_program(util.get_gl().make_program({
        "/shaders/scene.v.glsl",
        "/shaders/scene.f.glsl"}))
  , _world(map, y::ivec2(coord + y::wvec2{.5, .5}).euclidean_div(
        Tileset::tile_size * Cell::cell_size))
  , _collision(_world)
  , _lighting(_world, util.get_gl())
  , _environment(util.get_gl())
  , _camera_rotation(0)
  , _is_camera_moving_x(false)
  , _is_camera_moving_y(false)
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
  util.get_window().set_key_repeat(false);
  _key_map[KEY_UP] = {sf::Keyboard::W, sf::Keyboard::Up,
                      sf::Keyboard::LControl, sf::Keyboard::RControl};
  _key_map[KEY_DOWN] = {sf::Keyboard::S, sf::Keyboard::Down};
  _key_map[KEY_LEFT] = {sf::Keyboard::A, sf::Keyboard::Left};
  _key_map[KEY_RIGHT] = {sf::Keyboard::D, sf::Keyboard::Right};
}

GameStage::~GameStage()
{
  _util.get_gl().delete_program(_scene_program);
  _util.get_gl().delete_framebuffer(_colourbuffer);
  _util.get_gl().delete_framebuffer(_normalbuffer);
  _util.get_gl().delete_framebuffer(_lightbuffer);
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

const Lighting& GameStage::get_lighting() const
{
  return _lighting;
}

Lighting& GameStage::get_lighting()
{
  return _lighting;
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

  // Update scripts. Can't use iterators, or they will be invalidated by new
  // creations. Destructions won't happen here, though.
  for (y::size i = 0; i < _scripts.size(); ++i) {
    const auto& script = _scripts[i];
    if (script->has_function("update") && !script->is_destroyed()) {
      script->call("update");
    }
  }

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
      for (const auto& script : _scripts) {
        script->set_origin(script->get_origin() - script_move);
      }
      _camera -= script_move;
    }
  }

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

  // Clean up manually-destroyed scripts.
  _scripts.erase(
      std::remove_if(_scripts.begin(), _scripts.end(), local::is_destroyed),
      _scripts.end());
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

  // Render colour buffer.
  _current_batch.clear();
  _colourbuffer.bind(true, true);
  render_all(false);
  _util.render_batch(_current_batch);

  // Render normal buffer.
  _current_batch.clear();
  _normalbuffer.bind(true, true);
  render_all(true);
  _util.render_batch(_current_batch);

  // Render the scene by the lighting.
  _lightbuffer.bind(true, false);
  _lighting.render_lightbuffer(_util, _normalbuffer,
                               get_camera_min(), get_camera_max());

  // Draw scene with lighting.
  _framebuffer.bind(false, false);
  _util.get_gl().enable_depth(false);
  _util.get_gl().enable_blend(false);
  _scene_program.bind();
  _scene_program.bind_attribute("position", _util.quad_vertex());
  _scene_program.bind_uniform("colourbuffer", _colourbuffer);
  _scene_program.bind_uniform("lightbuffer", _lightbuffer);
  _util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);

  // Re-render colour and normal buffer for the environment.
  // TODO: make it a function.
  _environment.render(_util, get_camera_min(), get_camera_max(),
                      _colourbuffer, _normalbuffer);
  // Re-render the scene by the lighting.
  _lightbuffer.bind(true, false);
  _lighting.render_lightbuffer(_util, _normalbuffer,
                               get_camera_min(), get_camera_max());

  // Draw environment overlay with lighting.
  _framebuffer.bind(false, false);
  _util.get_gl().enable_depth(false);
  _util.get_gl().enable_blend(true);
  _scene_program.bind();
  // TODO: do we need to re-bind?
  _scene_program.bind_attribute("position", _util.quad_vertex());
  _scene_program.bind_uniform("colourbuffer", _colourbuffer);
  _scene_program.bind_uniform("lightbuffer", _lightbuffer);
  _util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);

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

RenderBatch& GameStage::get_current_batch() const
{
  return _current_batch;
}

bool GameStage::is_current_normal_buffer() const
{
  return _current_is_normal_buffer;
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

void GameStage::add_script(y::unique<Script> script)
{
  _scripts.emplace_back();
  (_scripts.end() - 1)->swap(script);
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

void GameStage::render_all(bool normal_buffer) const
{
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

        const GlTexture2D texture = normal_buffer ?
            t.tileset->get_texture().normal : t.tileset->get_texture().texture;

        _current_batch.add_sprite(
            texture, Tileset::tile_size, y::fvec2(world),
            t.tileset->from_index(t.index), d, 0.f, colour::white);
      }
    }
  }

  // Render all scripts which overlap the screen.
  _current_is_normal_buffer = normal_buffer;
  for (const auto& script : _scripts) {
    const y::wvec2 min = script->get_origin() - script->get_region() / 2;
    const y::wvec2 max = script->get_origin() + script->get_region() / 2;
    if (max > get_camera_min() && min < get_camera_max() &&
        script->has_function("draw")) {
      script->call("draw");
    }
  }
}
