#ifndef GAME__STAGE_H
#define GAME__STAGE_H

#include "collision.h"
#include "environment.h"
#include "lighting.h"
#include "savegame.h"
#include "world.h"

#include "../common.h"
#include "../lua.h"
#include "../modal.h"
#include "../render/util.h"
#include "../spatial_hash.h"

#include <SFML/Window.hpp>

class Camera;
class CellMap;
class Filesystem;
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
  void send_message(Script* script, const y::string& function_name,
                    const y::vector<LuaValue>& args);

  // Functions below here should only be called by GameStage or GameRenderer,
  // not from the Lua API.

  // Update the spatial hash. Must be called when the Script moves or changes
  // geometry.
  void update_spatial_hash(Script& source);

  // Functions for updating/rendering all the Scripts.
  void update_all() const;
  void handle_messages();
  void move_all(const y::wvec2& move, Collision& collision);
  void render_all(const Camera& camera) const;

  // Functions for updating the active Scripts based on changes to the active
  // window.
  void get_preserved_cells(WorldWindow& world);
  void clean_out_of_bounds(const Script& player,
                           const y::wvec2& lower, const y::wvec2& upper);
  void clean_destroyed();
  // The WorldSource passed here must continue to exist for as long as any
  // Scripts sourced from it exist in the game world.
  void create_in_bounds(const Databank& bank, const WorldSource& source,
                        const WorldWindow& window,
                        const y::wvec2& lower, const y::wvec2& upper);

private:

  void add_script(y::unique<Script> script);
  void cleanup_script(Script* script);

  GameStage& _stage;

  typedef y::list<y::unique<Script>> script_list;
  script_list _scripts;
  
  // Spatial hash of existing scripts.
  typedef SpatialHash<Script*, y::world, 2> spatial_hash;
  spatial_hash _spatial_hash;

  // Map from ScriptBlueprint keys to existing scripts.
  struct script_map_key {
    bool operator==(const script_map_key& key) const;
    bool operator!=(const script_map_key& key) const;
    const WorldSource& source;
    ScriptBlueprint blueprint;
  };
  struct script_map_hash {
    y::size operator()(const script_map_key& key) const;
  };
  typedef y::map<script_map_key, ConstScriptReference,
                 script_map_hash> script_map;
  script_map _script_map;

  mutable y::map<const Script*, y::size> _uid_map;
  mutable y::ordered_set<y::size> _uid_unused;

  struct message {
    Script* script;
    y::string function_name;
    y::vector<LuaValue> args;
  };
  y::vector<message> _messages;

  // Temporary per-frame data for storing which cells we need to preserve.
  bool _all_cells_preserved;
  WorldWindow::cell_list _preserved_cells;

};

// Architecture: rendering proceeds in a series of layers. Each layer consists
// of a colour pass and a normal pass (if required), which are then combined
// and rendered with one of several potential lighting algorithms.

// On each layer we call the draw() function of every active script. Scripts
// check the current layer, drawing if so desired. When we need a normal pass
// the process is repeated and the Scripts draw the corresponding normal data.
// For the world layer we also draw all the tiles.
class GameRenderer : public y::no_copy {
public:

  // All the defined layers, in order. Keep in sync with render.lua.
  // TODO: distortion layers or passes? Probably one right at the end is all
  // we need, rather than several in-between other layers.
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
    DRAW_OVERLAY2,
    DRAW_SPECULAR2,
    DRAW_FULLBRIGHT2,
  };

  GameRenderer(RenderUtil& util, const GlFramebuffer& framebuffer);

  RenderUtil& get_util() const;
  const GlFramebuffer& get_framebuffer() const;

  // Batches up all the sprites for a particular draw pass and renders them all
  // at once when the pass is done.
  RenderBatch& get_current_batch() const;

  // Must be called to indicated something has been drawn during this layer, and
  // we shouldn't skip it.
  void set_current_draw_any() const;

  // Determines whether the current pass is a colour or normal pass.
  bool draw_pass_is_normal() const;

  // Determines whether the current pass is part of the given layer.
  bool draw_pass_is_layer(draw_layer layer) const;

  // Implements the complete rendering pipeline. Should only be called by
  // GameStage.
  void render(
      const Camera& camera, const WorldWindow& world, const ScriptBank& scripts,
      const Lighting& lighting, const Collision& collision) const;

private:

  // Enums for each pass of each layer. To add a new layer, also add enums for
  // its passes here, and implement draw_pass_is_normal(), draw_pass_is_layer()
  // and draw_pass_light_type() for those passes.
  enum draw_pass {
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
    DRAW_OVERLAY2_NORMAL,
    DRAW_OVERLAY2_COLOUR,
    DRAW_SPECULAR2_NORMAL,
    DRAW_SPECULAR2_COLOUR,
    DRAW_FULLBRIGHT2_COLOUR,
    DRAW_PASS_MAX,
  };

  // Each draw layer combines the colour and normal passes with the lighting
  // using either the regular lighting algorithm, the specular lighting
  // algorithm, or no lighting, in which case the normal pass is not required.
  enum layer_light_type {
    LIGHT_TYPE_NORMAL,
    LIGHT_TYPE_SPECULAR,
    LIGHT_TYPE_FULLBRIGHT,
  };

  // Determines the lighting algorithm used for the current pass.
  layer_light_type draw_pass_light_type() const;

  // Render all the tiles in the world. Used in the DRAW_WORLD layer.
  void render_tiles(
      const Camera& camera, const WorldWindow& world) const;

  // Combine the various buffers and render the output of a layer.
  void render_scene(layer_light_type light_type) const;

  RenderUtil& _util;
  const GlFramebuffer& _framebuffer;

  GlUnique<GlFramebuffer> _colourbuffer;
  GlUnique<GlFramebuffer> _normalbuffer;
  GlUnique<GlFramebuffer> _lightbuffer;

  GlUnique<GlProgram> _scene_program;
  GlUnique<GlProgram> _scene_specular_program;
  mutable RenderBatch _current_batch;
  mutable draw_pass _current_draw_pass;
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

// Super God Object acts as the intermediary between Scripts and all game
// subsystems.
class GameStage : public Modal {
public:

  // Set fake to true to avoid expensive initialisation computations when
  // we're only using this as a fake to access Scripts from the editor.
  GameStage(const Databank& bank, Filesystem& save_filesystem,
            RenderUtil& util, const GlFramebuffer& framebuffer,
            const y::string& source_key, const y::wvec2& coord,
            bool fake = false);
  ~GameStage() override {};

  const Databank& get_bank() const;

  const Savegame& get_savegame() const;
  /***/ Savegame& get_savegame();

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

  // World source functions.
  // Since it's important we keep around all the sources we use, this function
  // should be used to access them based on a string key. The type of source
  // depends on the string, for example a path to a map file.
  const WorldSource& get_source(const y::string& source_key) const;

  // Modal functions.
  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

  // Lua API functions.
  void set_player(Script* script);
  Script* get_player() const;
  bool is_key_down(y::int32 key) const;
  void save_game() const;

private:

  void script_maps_clean_up();

  const Databank& _bank;
  Filesystem& _save_filesystem;

  typedef y::map<y::string, y::unique<const WorldSource>> source_map;
  mutable source_map _source_map;
  y::string _active_source_key;
  WorldWindow _world;

  Savegame _savegame;
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
