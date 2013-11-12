#include "bank.h"

#include "../filesystem/filesystem.h"
#include "../filesystem/physical.h"
#include "../game/stage.h"
#include "../lua.h"
#include "../render/gl_util.h"
#include "../render/util.h"
#include "../../gen/proto/cell.pb.h"

Databank::Databank()
  : _default_script{"/yedit/missing.lua", "", y::fvec4{1.f, 1.f, 1.f, 1.f}}
  , _default_tileset(_default_sprite)
  , scripts(_default_script)
  , sprites(_default_sprite)
  , tilesets(_default_tileset)
  , cells(_default_cell)
  , maps(_default_map)
{
}

Databank::Databank(const Filesystem& filesystem, GlUtil& gl,
                   bool load_yedit_data)
  : _default_script{"/yedit/missing.lua", "", y::fvec4{1.f, 1.f, 1.f, 1.f}}
  , _default_sprite{gl.make_texture("/yedit/missing.png"),
                    gl.make_texture("/default_normal.png")}
  , _default_tileset(_default_sprite)
  , scripts(_default_script)
  , sprites(_default_sprite)
  , tilesets(_default_tileset)
  , cells(_default_cell)
  , maps(_default_map)
{
  // Things should be loaded in order of dependence, so that the data can be
  // accessed while loading if necessary. For example, maps depend on cells
  // and scripts.
  y::string_vector paths;
  filesystem.list_pattern(paths, "/sprites/**.png");
  for (const y::string& s : paths) {
    y::string bare_path;
    filesystem.barename(bare_path, s);
    if (bare_path.substr(bare_path.length() - 7) == "_normal") {
      continue;
    }
    y::string normal_path = bare_path + "_normal.png";

    GlTexture2D texture = gl.make_texture(s);
    GlTexture2D normal_texture = filesystem.is_file(normal_path) ?
        gl.make_texture(normal_path) : _default_sprite.normal;

    _textures.emplace_back(gl.make_unique(texture));
    if (filesystem.is_file(normal_path)) {
      _textures.emplace_back(gl.make_unique(normal_texture));
    }

    sprites.insert(s, y::move_unique(new Sprite{texture, normal_texture}));
  }

  // Tilesets are available as sprites as well.
  paths.clear();
  filesystem.list_pattern(paths, "/tiles/**.png");
  for (const y::string& s : paths) {
    y::string bare_path;
    filesystem.barename(bare_path, s);
    if (bare_path.substr(bare_path.length() - 7) == "_normal") {
      continue;
    }
    y::string data_path = bare_path + ".tile";
    y::string normal_path = bare_path + "_normal.png";

    GlTexture2D texture = gl.make_texture(s);
    GlTexture2D normal_texture = filesystem.is_file(normal_path) ?
        gl.make_texture(normal_path) : _default_sprite.normal;

    _textures.emplace_back(gl.make_unique(texture));
    if (filesystem.is_file(normal_path)) {
      _textures.emplace_back(gl.make_unique(normal_texture));
    }

    Tileset* tileset = new Tileset({texture, normal_texture});
    if (filesystem.exists(data_path)) {
      tileset->load(filesystem, *this, data_path);
    }
    tilesets.insert(data_path, y::move_unique(tileset));
    sprites.insert(s, y::move_unique(new Sprite{texture, normal_texture}));
  }

  filesystem.read_file_with_includes(_default_script.contents,
                                     _default_script.path);
  // Scripts in the root directory are for inclusion only.
  paths.clear();
  filesystem.list_pattern(paths, "/scripts/*/**.lua");
  for (const y::string& s : paths) {
    LuaFile* lua_file = new LuaFile;
    lua_file->path = s;
    filesystem.read_file_with_includes(lua_file->contents, s);
    scripts.insert(s, y::move_unique(lua_file));
  }

  // Maps depend on knowing the names of scripts, but grabbing data out of
  // scripts requires an actual map to be loaded for the GameStage, so this
  // needs to happen in-between.
  reload_cells_and_maps(filesystem);

  // Fill in the editor data of the scripts. Construct a fake GameStage so we
  // can access the functions.
  if (load_yedit_data) {
    RenderUtil fake_util(gl);
    GlUnique<GlFramebuffer> fake_framebuffer(
        gl.make_unique_framebuffer(y::ivec2{128, 128}, false, false));
    PhysicalFilesystem fake_filesystem("fake");
    GameStage fake_stage(*this, fake_filesystem, fake_util, *fake_framebuffer,
                         maps.get_names()[0], y::wvec2(), true);
    for (y::size i = 0; i < scripts.size(); ++i) {
      make_lua_file(scripts.get(i), fake_stage);
    }
  }
}

void Databank::reload_cells_and_maps(const Filesystem& filesystem)
{
  cells.clear();
  maps.clear();

  y::string_vector paths;
  filesystem.list_pattern(paths, "/world/**.cell");
  for (const y::string& s : paths) {
    CellBlueprint* cell = new CellBlueprint();
    cell->load(filesystem, *this, s);
    cells.insert(s, y::move_unique(cell));
  }

  paths.clear();
  filesystem.list_pattern(paths, "/world/**.map");
  for (const y::string& s : paths) {
    CellMap* map = new CellMap();
    map->load(filesystem, *this, s);
    maps.insert(s, y::move_unique(map));
  }

  if (cells.get_names().empty() || maps.get_names().empty()) {
    make_default_map();
  }
}

void Databank::make_default_map()
{
  CellBlueprint* blueprint = new CellBlueprint();
  CellMap* map = new CellMap();
  map->set_coord(y::ivec2(), *blueprint);

  cells.insert("/world/default.cell", y::move_unique(blueprint));
  maps.insert("/world/default.map", y::move_unique(map));
}

void Databank::make_lua_file(LuaFile& file, GameStage& fake_stage)
{
  Script script(fake_stage, file.path, file.contents, y::wvec2(), y::wvec2());

  file.yedit_colour = y::fvec4{.5f, .5f, .5f, 1.f};
  if (script.has_function("yedit_colour")) {
    Script::lua_args out;
    script.call(out, "yedit_colour");
    if (out.size() >= 3) {
      file.yedit_colour[rr] = float(out[0].world);
      file.yedit_colour[gg] = float(out[1].world);
      file.yedit_colour[bb] = float(out[2].world);
    }
  }
}
