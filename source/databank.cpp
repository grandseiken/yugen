#include "databank.h"
#include "filesystem.h"
#include "gl_util.h"
#include "proto/cell.pb.h"

Databank::Databank(const Filesystem& filesystem, GlUtil& gl)
  : _default_script(filesystem, "/yedit/missing.lua")
  , _default_tileset(gl.make_texture("/yedit/missing.png"))
  , scripts(_default_script)
  , tilesets(_default_tileset)
  , cells(_default_cell)
  , maps(_default_map)
{
  y::string_vector paths;
  filesystem.list_pattern(paths, "/scripts/**.lua");
  for (const y::string& s : paths) {
    scripts.insert(s, y::move_unique(new Script(filesystem, s)));
  }

  paths.clear();
  filesystem.list_pattern(paths, "/tiles/**.png");
  for (const y::string& s : paths) {
    tilesets.insert(s, y::move_unique(new Tileset(gl.make_texture(s))));
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
