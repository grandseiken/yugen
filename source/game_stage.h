#ifndef GAME_STAGE_H
#define GAME_STAGE_H

#include "collision.h"
#include "common.h"
#include "lua.h"
#include "modal.h"
#include "render_util.h"
#include "world.h"

class CellMap;
class GlFramebuffer;
class RenderUtil;
struct LuaFile;

class GameStage : public Modal {
public:

  GameStage(const Databank& bank,
            RenderUtil& util, GlFramebuffer& framebuffer,
            const CellMap& map, const y::ivec2& coord);
  ~GameStage() override;

  const Databank& get_bank() const;
  RenderUtil& get_util() const;
  const Collision& get_collision() const;
  /***/ Collision& get_collision();
  const y::wvec2& get_camera() const;

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

  // Lua API functions.
  void destroy_script(const Script& script);
  Script& create_script(
      const LuaFile& file, const y::wvec2& origin);
  Script& create_script(
      const LuaFile& file,
      const y::wvec2& origin, const y::wvec2& region);
  RenderBatch& get_current_batch();

private:

  y::wvec2 world_to_camera(const y::wvec2& v) const;
  y::wvec2 camera_to_world(const y::wvec2& v) const;

  void add_script(y::unique<Script> script);

  const Databank& _bank;
  RenderUtil& _util;
  GlFramebuffer& _framebuffer;
  const CellMap& _map;

  WorldWindow _world;
  Collision _collision;
  y::wvec2 _camera;

  typedef y::vector<y::unique<Script>> script_list;
  script_list _scripts;

  mutable RenderBatch _current_batch;

};

#endif
