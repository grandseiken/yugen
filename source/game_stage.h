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

class CellMap;
class RenderUtil;
struct LuaFile;

// Stores all the scripts currently active.
class ScriptBank : public y::no_copy {
public:

  ScriptBank(GameStage& stage);

  Script& create_script(
      const LuaFile& file, const y::wvec2& origin);
  Script& create_script(
      const LuaFile& file,
      const y::wvec2& origin, const y::wvec2& region);

private:

  friend class GameStage;
  void add_script(y::unique<Script> script);

  void update_all() const;
  void move_all(const y::wvec2& move) const;
  void render_all(const y::wvec2& camera_min, const y::wvec2& camera_max) const;

  void get_unrefreshed(WorldWindow& world);
  void clean_out_of_bounds(const Script& player,
                           const y::wvec2& lower, const y::wvec2& upper);
  void clean_destroyed();
  void create_in_bounds(const Databank& bank, const CellMap& map,
                        const WorldWindow& window,
                        const y::wvec2& lower, const y::wvec2& upper);

  GameStage& _stage;

  typedef y::vector<y::unique<Script>> script_list;
  script_list _scripts;
  bool _all_unrefreshed;
  WorldWindow::cell_list _unrefreshed;

};

// TODO: this is getting monolithic. Split out... camera? Rendering?
class GameStage : public Modal {
public:

  GameStage(const Databank& bank,
            RenderUtil& util, const GlFramebuffer& framebuffer,
            const CellMap& map, const y::wvec2& coord);
  ~GameStage() override {};

  const Databank& get_bank() const;
  RenderUtil& get_util() const;

  const ScriptBank& get_scripts() const;
  /***/ ScriptBank& get_scripts();

  const Collision& get_collision() const;
  /***/ Collision& get_collision();

  const Lighting& get_lighting() const;
  /***/ Lighting& get_lighting();

  y::wvec2 world_to_camera(const y::wvec2& v) const;
  y::wvec2 camera_to_world(const y::wvec2& v) const;

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

  RenderBatch& get_current_batch() const;
  bool is_current_normal_buffer() const;

  // Lua API functions.
  void set_player(Script* script);
  Script* get_player() const;
  bool is_key_down(y::int32 key) const;
  void set_camera(const y::wvec2& camera);
  const y::wvec2& get_camera() const;
  void set_camera_rotation(y::world rotation);
  y::world get_camera_rotation() const;

private:

  void script_maps_clean_up();

  void update_camera(Script* focus);
  y::wvec2 get_camera_min() const;
  y::wvec2 get_camera_max() const;

  void render_all(bool normal_buffer) const;
  void render_scene(bool enable_blend) const;

  const Databank& _bank;
  RenderUtil& _util;
  const CellMap& _map;

  const GlFramebuffer& _framebuffer;
  GlUnique<GlFramebuffer> _colourbuffer;
  GlUnique<GlFramebuffer> _normalbuffer;
  GlUnique<GlFramebuffer> _lightbuffer;
  GlUnique<GlProgram> _scene_program;

  WorldWindow _world;
  ScriptBank _scripts;
  Collision _collision;
  Lighting _lighting;
  Environment _environment;

  y::wvec2 _camera;
  y::world _camera_rotation;
  bool _is_camera_moving_x;
  bool _is_camera_moving_y;
  Script* _player;

  typedef y::map<y::int32, y::set<y::int32>> key_map;
  key_map _key_map;

  mutable RenderBatch _current_batch;
  mutable bool _current_is_normal_buffer;

};

#endif
