#include "game_stage.h"

#include "databank.h"
#include "gl_util.h"
#include "tileset.h"
#include "window.h"

ScriptBank::ScriptBank(GameStage& stage)
  : _stage(stage)
  , _uid_counter(0)
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

void ScriptBank::get_in_region(
    result& output,
    const y::wvec2& origin, const y::wvec2& region) const
{
  // TODO: we should be able to accelerate this somehow. Looping through
  // every time is bad. Not as easy as it seems, though, since to skip by x-axis
  // we need to re-sort every time this function is called (as the scripts might
  // move around between calls), which is at least O(n) even on an almost-sorted
  // list anyway.
  // We need to either accept up to one frame of staleness, or think up some
  // cleverer acceleration structure.
  for (const auto& script : _scripts) {
    if (script->get_origin().in_region(origin - region / 2, region)) {
      output.emplace_back(script.get());
    }
  }
}

void ScriptBank::get_in_radius(result& output,
                               const y::wvec2& origin, y::world radius) const
{
  const y::world radius_sq = radius * radius;
  for (const auto& script : _scripts) {
    if ((script->get_origin() - origin).length_squared() <= radius_sq) {
      output.emplace_back(script.get());
    }
  }
}

y::int32 ScriptBank::get_uid(const Script* script) const
{
  // TODO: this is really naive, but works for now.
  auto it = _uid_map.find(script);
  if (it != _uid_map.end()) {
    return it->second;
  }
  _uid_map.insert(y::make_pair(script, _uid_counter));
  return _uid_counter++;
}

void ScriptBank::send_message(Script* script, const y::string& function_name,
                              const y::vector<LuaValue>& args)
{
  _messages.push_back({script, function_name, args});
}

void ScriptBank::add_script(y::unique<Script> script)
{
  _scripts.emplace_back();
  (_scripts.rbegin())->swap(script);
}

void ScriptBank::update_all() const
{
  // Destructions won't happen here. Iteration is fine even though new
  // scripts may be added in the loop, since we're using a doubly-linked
  // list.
  for (const auto& script : _scripts) {
    if (script->has_function("update") && !script->is_destroyed()) {
      script->call("update");
    }
  }
}

void ScriptBank::handle_messages()
{
  for (const auto& m : _messages) {
    if (m.script->has_function(m.function_name)) {
      m.script->call(m.function_name, m.args);
    }
  }
  _messages.clear();
}

void ScriptBank::move_all(const y::wvec2& move) const
{
  for (const auto& script : _scripts) {
    script->set_origin(script->get_origin() + move);
  }
}

void ScriptBank::render_all(const Camera& camera) const
{
  for (const auto& script : _scripts) {
    const y::wvec2 min = script->get_origin() - script->get_region() / 2;
    const y::wvec2 max = script->get_origin() + script->get_region() / 2;
    if (max > camera.get_min() && min < camera.get_max() &&
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

    // If the script overlaps at all with any cell which is not being refreshed,
    // then we need to keep it around. If it is entirely contained within cells
    // we're replacing, or genuine out-of-bounds areas, then get rid of it.
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
  _scripts.remove_if(is_out_of_bounds);
}

void ScriptBank::clean_destroyed()
{
  struct local {
    static bool is_destroyed(const y::unique<Script>& s)
    {
      return s->is_destroyed();
    }
  };

  _scripts.remove_if(local::is_destroyed);
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

  // Create scripts that overlap new cells (which were not already active).
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

GameRenderer::GameRenderer(RenderUtil& util, const GlFramebuffer& framebuffer)
  : _util(util)
  , _framebuffer(framebuffer)
  , _colourbuffer(util.get_gl().make_unique_framebuffer(
        framebuffer.get_size(), true, true))
  , _normalbuffer(util.get_gl().make_unique_framebuffer(
        framebuffer.get_size(), false, true))
  , _lightbuffer(util.get_gl().make_unique_framebuffer(
        framebuffer.get_size(), false, false))
  , _scene_program(util.get_gl().make_unique_program({
        "/shaders/scene.v.glsl",
        "/shaders/scene.f.glsl"}))
  , _scene_specular_program(util.get_gl().make_unique_program({
        "/shaders/scene_specular.v.glsl",
        "/shaders/scene_specular.f.glsl"}))
  , _current_draw_pass(draw_pass(0))
{
}

RenderUtil& GameRenderer::get_util() const
{
  return _util;
}

const GlFramebuffer& GameRenderer::get_framebuffer() const
{
  return _framebuffer;
}

RenderBatch& GameRenderer::get_current_batch() const
{
  return _current_batch;
}

void GameRenderer::set_current_draw_any() const
{
  _current_draw_any = true;
}

bool GameRenderer::draw_pass_is_normal() const
{
  const draw_pass& pass = _current_draw_pass;
  return pass == DRAW_UNDERLAY0_NORMAL ||
         pass == DRAW_UNDERLAY1_NORMAL ||
         pass == DRAW_WORLD_NORMAL ||
         pass == DRAW_OVERLAY0_NORMAL ||
         pass == DRAW_SPECULAR0_NORMAL ||
         pass == DRAW_OVERLAY1_NORMAL ||
         pass == DRAW_SPECULAR1_NORMAL;
}

bool GameRenderer::draw_pass_is_layer(draw_layer layer) const
{
  const draw_pass& pass = _current_draw_pass;
  switch (layer) {
    case DRAW_PARALLAX0:
      return pass == DRAW_PARALLAX0_COLOUR;
    case DRAW_PARALLAX1:
      return pass == DRAW_PARALLAX1_COLOUR;
    case DRAW_UNDERLAY0:
      return pass == DRAW_UNDERLAY0_NORMAL || pass == DRAW_UNDERLAY0_COLOUR;
    case DRAW_UNDERLAY1:
      return pass == DRAW_UNDERLAY1_NORMAL || pass == DRAW_UNDERLAY1_COLOUR;
    case DRAW_WORLD:
      return pass == DRAW_WORLD_NORMAL || pass == DRAW_WORLD_COLOUR;
    case DRAW_OVERLAY0:
      return pass == DRAW_OVERLAY0_NORMAL || pass == DRAW_OVERLAY0_COLOUR;
    case DRAW_SPECULAR0:
      return pass == DRAW_SPECULAR0_NORMAL || pass == DRAW_SPECULAR0_COLOUR;
    case DRAW_FULLBRIGHT0:
      return pass == DRAW_FULLBRIGHT0_COLOUR;
    case DRAW_OVERLAY1:
      return pass == DRAW_OVERLAY1_NORMAL || pass == DRAW_OVERLAY1_COLOUR;
    case DRAW_SPECULAR1:
      return pass == DRAW_SPECULAR1_NORMAL || pass == DRAW_SPECULAR1_COLOUR;
    case DRAW_FULLBRIGHT1:
      return pass == DRAW_FULLBRIGHT1_COLOUR;
    default:
      return false;
  }
}

void GameRenderer::render(
    const Camera& camera, const WorldWindow& world, const ScriptBank& scripts,
    const Lighting& lighting, const Collision& collision) const
{
  y::fvec2 translation = y::fvec2(camera.world_to_camera(y::wvec2()));
  _util.add_translation(translation);
  _framebuffer.bind(true, true);

  // Loop through the draw passes. Each pass is one loop through everything
  // that can be rendered, but we combine several passes into one semantic
  // layer. For example, most layers need both a colour pass and a normal or
  // metadata pass to assemble the final result.
  for (_current_draw_pass = draw_pass(0);
       _current_draw_pass < DRAW_PASS_MAX;
       _current_draw_pass = draw_pass(1 + _current_draw_pass)) {
    // Render colour buffer or normal buffer as appropriate.
    _current_batch.clear();
    _current_draw_any = false;
    if (draw_pass_is_normal()) {
      _normalbuffer->bind(false, true);
    }
    else {
      _colourbuffer->bind(true, true);
    }

    if (draw_pass_is_layer(DRAW_WORLD)) {
      render_tiles(camera, world);
    }
    scripts.render_all(camera);
    _util.render_batch(_current_batch);

    // If there's anything on this layer, render scene by the lighting. We know
    // if it's the end of a layer by the (currently hacky) method of checking
    // whether this pass is a colour pass, which are always present and last by
    // design.
    if (!draw_pass_is_normal() && _current_draw_any) {
      layer_light_type light_type(draw_pass_light_type());
      // Swap the rendering method based on the semantic type of the layer.
      if (light_type == LIGHT_TYPE_NORMAL) {
        _lightbuffer->bind(true, false);
        lighting.render_lightbuffer(_util, *_normalbuffer,
                                     camera.get_min(), camera.get_max());
      }
      else if (light_type == LIGHT_TYPE_SPECULAR) {
        _lightbuffer->bind(true, false);
        lighting.render_specularbuffer(_util, *_normalbuffer,
                                        camera.get_min(), camera.get_max());
      }
      else {
        _lightbuffer->bind(false, false);
        _util.clear({1.f, 1.f, 1.f, 1.f});
      }

      _framebuffer.bind(false, false);
      render_scene(true, light_type == LIGHT_TYPE_SPECULAR);
    }
  }
  _framebuffer.bind(false, false);

  // Render geometry.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::G)) {
    collision.render(_util, camera.get_min(), camera.get_max());
  }

  // Render lighting.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::L)) {
    lighting.render_traces(_util, camera.get_min(), camera.get_max());
  }

  _util.add_translation(-translation);
}

GameRenderer::layer_light_type GameRenderer::draw_pass_light_type() const
{
  draw_pass pass = _current_draw_pass;
  if (pass == DRAW_SPECULAR0_NORMAL || pass == DRAW_SPECULAR0_COLOUR ||
      pass == DRAW_SPECULAR1_NORMAL || pass == DRAW_SPECULAR1_COLOUR) {
    return LIGHT_TYPE_SPECULAR;
  }
  if (pass == DRAW_FULLBRIGHT0_COLOUR || pass == DRAW_FULLBRIGHT1_COLOUR ||
      pass == DRAW_PARALLAX0_COLOUR || pass == DRAW_PARALLAX1_COLOUR) {
    return LIGHT_TYPE_FULLBRIGHT;
  }
  return LIGHT_TYPE_NORMAL;
}

void GameRenderer::render_tiles(
    const Camera& camera, const WorldWindow& world) const
{
  _current_draw_any = true;

  // Render all the tiles in the world at once, batched by texture.
  for (auto it = world.get_cartesian(); it; ++it) {
    Cell* cell = world.get_active_window_cell(*it);
    if (!cell) {
      continue;
    }
    for (auto jt = y::cartesian(Cell::cell_size); jt; ++jt) {
      y::ivec2 w = Tileset::tile_size * (*jt + *it * Cell::cell_size);
      if (y::wvec2(w + Tileset::tile_size) <= camera.get_min() ||
          y::wvec2(w) >= camera.get_max()) {
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
        bool normal = draw_pass_is_normal();

        const GlTexture2D texture = normal ?
            t.tileset->get_texture().normal : t.tileset->get_texture().texture;

        _current_batch.add_sprite(
            texture, Tileset::tile_size, normal,
            y::fvec2(w), t.tileset->from_index(t.index),
            d, 0.f, colour::white);
      }
    }
  }
}

void GameRenderer::render_scene(bool enable_blend, bool specular) const
{
  _framebuffer.bind(false, false);
  _util.get_gl().enable_depth(false);
  _util.get_gl().enable_blend(enable_blend);
  const GlProgram& program(specular ? *_scene_specular_program :
                                      *_scene_program);
  program.bind();
  program.bind_attribute("position", _util.quad_vertex());
  program.bind_uniform("colourbuffer", *_colourbuffer);
  program.bind_uniform("lightbuffer", *_lightbuffer);
  _util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}

Camera::Camera(const y::ivec2& framebuffer_size)
  : _framebuffer_size(framebuffer_size)
  , _rotation(0)
  , _is_moving_x(false)
  , _is_moving_y(false)
{
}

void Camera::update(Script* focus)
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

  if (y::abs(target[xx] - _origin[xx]) < camera_deadzone[xx] / 2) {
    _is_moving_x = false;
  }
  if (y::abs(target[yy] - _origin[yy]) < camera_deadzone[yy] / 2) {
    _is_moving_y = false;
  }

  if (y::abs(target[xx] - _origin[xx]) > camera_deadzone_buffer[xx] / 2) {
    _is_moving_x = true;
  }
  if (y::abs(target[yy] - _origin[yy]) > camera_deadzone_buffer[yy] / 2) {
    _is_moving_y = true;
  }

  y::wvec2 dir = target - _origin;
  if (_is_moving_x && dir[xx]) {
    _origin[xx] += camera_speed[xx] * (dir[xx] / y::abs(dir[xx]));
  }
  if (_is_moving_y && dir[yy]) {
    _origin[yy] += camera_speed[yy] * (dir[yy] / y::abs(dir[yy]));
  }
}

void Camera::move(const y::wvec2& move)
{
  _origin += move;
}

void Camera::set_origin(const y::wvec2& origin)
{
  _origin = origin;
}

void Camera::set_rotation(y::world rotation)
{
  _rotation = rotation;
}

y::wvec2 Camera::world_to_camera(const y::wvec2& v) const
{
  return v - _origin + y::wvec2(_framebuffer_size) / 2;
}

y::wvec2 Camera::camera_to_world(const y::wvec2& v) const
{
  return v - y::wvec2(_framebuffer_size) / 2 + _origin;
}

y::wvec2 Camera::get_min() const
{
  return camera_to_world(y::wvec2());
}

y::wvec2 Camera::get_max() const
{
  return camera_to_world(y::wvec2(_framebuffer_size));
}

const y::wvec2& Camera::get_origin() const
{
  return _origin;
}

y::world Camera::get_rotation() const
{
  return _rotation;
}

GameStage::GameStage(const Databank& bank,
                     RenderUtil& util, const GlFramebuffer& framebuffer,
                     const CellMap& map, const y::wvec2& coord)
  : _bank(bank)
  , _map(map)
  , _world(map, y::ivec2(coord + y::wvec2{.5, .5}).euclidean_div(
        Tileset::tile_size * Cell::cell_size))
  , _scripts(*this)
  , _renderer(util, framebuffer)
  , _camera(framebuffer.get_size())
  , _collision(_world)
  , _lighting(_world, util.get_gl())
  , _environment(util.get_gl())
  , _player(y::null)
{
  const LuaFile& file = _bank.scripts.get("/scripts/game/player.lua");
  y::wvec2 offset = y::wvec2(_world.get_active_coord() *
                             Cell::cell_size * Tileset::tile_size);
  Script& player = _scripts.create_script(file, coord - offset);
  set_player(&player);
  _camera.move(coord - offset);

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

const ScriptBank& GameStage::get_scripts() const
{
  return _scripts;
}

ScriptBank& GameStage::get_scripts()
{
  return _scripts;
}

const GameRenderer& GameStage::get_renderer() const
{
  return _renderer;
}

GameRenderer& GameStage::get_renderer()
{
  return _renderer;
}

const Camera& GameStage::get_camera() const
{
  return _camera;
}

Camera& GameStage::get_camera()
{
  return _camera;
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
      y::vector<LuaValue> args;
      args.emplace_back(y::world(pair.first));
      get_player()->call("key", args);
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
  _scripts.handle_messages();

  // Update window. When we need to move the active window, make sure to
  // compensate by moving all scripts and the camera to balance it out.
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
      _camera.move(-script_move);
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
    _camera.update(get_player());
  }

  // Recalculate lighting.
  _lighting.recalculate_traces(_camera.get_min(), _camera.get_max());
}

void GameStage::draw() const
{
  _renderer.render(_camera, _world, _scripts, _lighting, _collision);
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

void GameStage::script_maps_clean_up()
{
  _collision.clean_up();
  _lighting.clean_up();
}
