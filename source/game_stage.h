#ifndef GAME_STAGE_H
#define GAME_STAGE_H

#include "collision.h"
#include "common.h"
#include "environment.h"
#include "lighting.h"
#include "lua.h"
#include "modal.h"
#include "render_util.h"
#include "world.h"

#include <SFML/Window.hpp>

class Camera;
class CellMap;
class RenderUtil;
struct LuaFile;

// Stores all the scripts currently active.
// TODO: this uses the odd convention of exposing public to external clients,
// but requiring classes in the same file to access privates using friend.
// Everything or nothing in thie file should work the same way. The root cause
// is the distinction between C++ engine access and Lua script access.
class ScriptBank : public y::no_copy {
public:

  ScriptBank(GameStage& stage);

  Script& create_script(
      const LuaFile& file, const y::wvec2& origin);
  Script& create_script(
      const LuaFile& file,
      const y::wvec2& origin, const y::wvec2& region);

  typedef y::vector<Script*> result;
  // Finds scripts whose origin lies in the given region. Ignores assigned
  // region of the script itself.
  void get_in_region(result& output,
                     const y::wvec2& origin, const y::wvec2& region) const;
  // Similar, but for scripts whose origin lies in the given circle.
  void get_in_radius(result& output,
                     const y::wvec2& origin, y::world radius) const;

  // Lua userdata can't be hashed, and since we make a new userdata each time
  // we give a Script to Lua we need a way of indexing tables. Even if we gave
  // the same userdata it would have to be modulo the calling Script; this
  // ensures different Scripts can be uniquely identified across different
  // client Scripts.
  y::int32 get_uid(const Script* script) const;

  // Stash a message for calling at the end of the frame.
  void send_message(Script* script, const y::string& function_name);

private:

  friend class GameStage;
  friend class GameRenderer;
  void add_script(y::unique<Script> script);

  void update_all() const;
  void handle_messages();
  void move_all(const y::wvec2& move) const;
  void render_all(const Camera& camera) const;

  void get_unrefreshed(WorldWindow& world);
  void clean_out_of_bounds(const Script& player,
                           const y::wvec2& lower, const y::wvec2& upper);
  void clean_destroyed();
  void create_in_bounds(const Databank& bank, const CellMap& map,
                        const WorldWindow& window,
                        const y::wvec2& lower, const y::wvec2& upper);

  GameStage& _stage;

  typedef y::list<y::unique<Script>> script_list;
  script_list _scripts;

  mutable y::map<const Script*, y::size> _uid_map;
  mutable y::int32 _uid_counter;

  typedef y::pair<Script*, y::string> message;
  y::vector<message> _messages;

  bool _all_unrefreshed;
  WorldWindow::cell_list _unrefreshed;

};

// Handles all rendering for the GameStage.
class GameRenderer : public y::no_copy {
public:

  // Keep in sync with render.lua.
  enum draw_layer {
    DRAW_PARALLAX0,
    DRAW_PARALLAX1,
    DRAW_UNDERLAY0,
    DRAW_UNDERLAY1,
    DRAW_WORLD,
    DRAW_OVERLAY0,
    DRAW_SPECULAR0,
    DRAW_FULLBRIGHT0,
    DRAW_OVERLAY1,
    DRAW_SPECULAR1,
    DRAW_FULLBRIGHT1,
  };

  GameRenderer(RenderUtil& util, const GlFramebuffer& framebuffer);

  RenderUtil& get_util() const;
  const GlFramebuffer& get_framebuffer() const;

  RenderBatch& get_current_batch() const;
  void set_current_draw_any() const;
  bool draw_stage_is_normal() const;
  bool draw_stage_is_layer(draw_layer layer) const;

  void render(
      const Camera& camera, const WorldWindow& world, const ScriptBank& scripts,
      const Lighting& lighting, const Collision& collision) const;

private:

  enum draw_stage {
    DRAW_PARALLAX0_COLOUR,
    DRAW_PARALLAX1_COLOUR,
    DRAW_UNDERLAY0_NORMAL,
    DRAW_UNDERLAY0_COLOUR,
    DRAW_UNDERLAY1_NORMAL,
    DRAW_UNDERLAY1_COLOUR,
    DRAW_WORLD_NORMAL,
    DRAW_WORLD_COLOUR,
    DRAW_OVERLAY0_NORMAL,
    DRAW_OVERLAY0_COLOUR,
    DRAW_SPECULAR0_NORMAL,
    DRAW_SPECULAR0_COLOUR,
    DRAW_FULLBRIGHT0_COLOUR,
    DRAW_OVERLAY1_NORMAL,
    DRAW_OVERLAY1_COLOUR,
    DRAW_SPECULAR1_NORMAL,
    DRAW_SPECULAR1_COLOUR,
    DRAW_FULLBRIGHT1_COLOUR,
    DRAW_STAGE_MAX,
  };
  enum layer_light_type {
    LIGHT_TYPE_NORMAL,
    LIGHT_TYPE_SPECULAR,
    LIGHT_TYPE_FULLBRIGHT,
  };
  layer_light_type draw_stage_light_type(draw_stage stage) const;

  void render_tiles(
      const Camera& camera, const WorldWindow& world) const;
  void render_scene(bool enable_blend, bool specular) const;

  RenderUtil& _util;
  const GlFramebuffer& _framebuffer;

  GlUnique<GlFramebuffer> _colourbuffer;
  GlUnique<GlFramebuffer> _normalbuffer;
  GlUnique<GlFramebuffer> _lightbuffer;
  GlUnique<GlProgram> _scene_program;
  GlUnique<GlProgram> _scene_specular_program;
  mutable RenderBatch _current_batch;
  mutable draw_stage _current_draw_stage;
  mutable bool _current_draw_any;

};

class Camera : public y::no_copy {
public:

  Camera(const y::ivec2& framebuffer_size);

  void update(Script* focus);
  void move(const y::wvec2& move);
  void set_origin(const y::wvec2& origin);
  void set_rotation(y::world rotation);

  y::wvec2 world_to_camera(const y::wvec2& v) const;
  y::wvec2 camera_to_world(const y::wvec2& v) const;

  y::wvec2 get_min() const;
  y::wvec2 get_max() const;
  const y::wvec2& get_origin() const;
  y::world get_rotation() const;

private:

  y::ivec2 _framebuffer_size;
  y::wvec2 _origin;
  y::world _rotation;
  bool _is_moving_x;
  bool _is_moving_y;

};

class GameStage : public Modal {
public:

  GameStage(const Databank& bank,
            RenderUtil& util, const GlFramebuffer& framebuffer,
            const CellMap& map, const y::wvec2& coord);
  ~GameStage() override {};

  const Databank& get_bank() const;

  const ScriptBank& get_scripts() const;
  /***/ ScriptBank& get_scripts();

  const GameRenderer& get_renderer() const;
  /***/ GameRenderer& get_renderer();

  const Camera& get_camera() const;
  /***/ Camera& get_camera();

  const Collision& get_collision() const;
  /***/ Collision& get_collision();

  const Lighting& get_lighting() const;
  /***/ Lighting& get_lighting();

  const Environment& get_environment() const;
  /***/ Environment& get_environment();

  // Modal functions.
  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

  // Lua API functions.
  void set_player(Script* script);
  Script* get_player() const;
  bool is_key_down(y::int32 key) const;

private:

  void script_maps_clean_up();

  const Databank& _bank;
  const CellMap& _map;

  WorldWindow _world;
  ScriptBank _scripts;
  GameRenderer _renderer;
  Camera _camera;
  Collision _collision;
  Lighting _lighting;
  Environment _environment;

  Script* _player;

  typedef y::map<y::int32, y::set<y::int32>> key_map;
  key_map _key_map;

};

#endif
