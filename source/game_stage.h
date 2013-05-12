#ifndef GAME_STAGE_H
#define GAME_STAGE_H

#include "common.h"
#include "lua.h"
#include "modal.h"
#include "render_util.h"
#include "world.h"

class CellMap;
class GlFramebuffer;
class RenderUtil;
struct LuaFile;

// TODO: make a typedef (and corresponding vec2) for in-game subpixel coordinate
// systems and use it wherever appropriate. (Double?)
class GameStage : public Modal {
public:

  GameStage(const Databank& bank,
            RenderUtil& util, GlFramebuffer& framebuffer,
            const CellMap& map, const y::ivec2& coord);
  virtual ~GameStage();

  const Databank& get_bank() const;
  RenderUtil& get_util() const;

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

  // Lua API functions.
  void destroy_script(const Script& script);
  Script& create_script(
      const LuaFile& file, const y::ivec2& origin);
  Script& create_script(
      const LuaFile& file,
      const y::ivec2& origin, const y::ivec2& region);
  RenderBatch& get_current_batch();

private:

  y::ivec2 world_to_camera(const y::ivec2& v) const;
  y::ivec2 camera_to_world(const y::ivec2& v) const;

  void add_script(y::unique<Script> script);

  const Databank& _bank;
  RenderUtil& _util;
  GlFramebuffer& _framebuffer;
  const CellMap& _map;

  WorldWindow _world;
  y::ivec2 _camera;

  typedef y::vector<y::unique<Script>> script_list;
  script_list _scripts;

  mutable RenderBatch _current_batch;

};

#endif
