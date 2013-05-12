#include "databank.h"
#include "filesystem.h"
#include "gl_util.h"
#include "proto/cell.pb.h"

Databank::Databank(const Filesystem& filesystem, GlUtil& gl)
  : _default_script{"/yedit/missing.lua", ""}
  , _default_sprite(gl.make_texture("/yedit/missing.png"))
  , _default_tileset(_default_sprite)
  , scripts(_default_script)
  , sprites(_default_sprite)
  , tilesets(_default_tileset)
  , cells(_default_cell)
  , maps(_default_map)
{
  filesystem.read_file_with_includes(_default_script.contents,
                                     _default_script.path);

  y::string_vector paths;
  filesystem.list_pattern(paths, "/scripts/**.lua");
  for (const y::string& s : paths) {
    LuaFile* lua_file = new LuaFile;
    lua_file->path = s;
    filesystem.read_file_with_includes(lua_file->contents, s);
    scripts.insert(s, y::move_unique(lua_file));
  }

  paths.clear();
  filesystem.list_pattern(paths, "/tiles/**.png");
  for (const y::string& s : paths) {
    y::string data_path;
    filesystem.barename(data_path, s);
    data_path += ".tile";
    Tileset* tileset = new Tileset(gl.make_texture(s));
    if (filesystem.exists(data_path)) {
      tileset->load(filesystem, *this, data_path);
    }
    tilesets.insert(data_path, y::move_unique(tileset));
  }

  // Don't clear; make tilesets accessible as sprites too.
  filesystem.list_pattern(paths, "/sprites/**.png");
  for (const y::string& s : paths) {
    GlTexture t = gl.get_texture(s);
    if (!t) {
      t = gl.make_texture(s);
    }
    sprites.insert(s, y::move_unique(new GlTexture(t)));
  }

  paths.clear();
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
