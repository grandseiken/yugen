#include "databank.h"
#include "filesystem.h"
#include "gl_util.h"

#include <algorithm>

Databank::Databank(const Filesystem& filesystem, GlUtil& gl)
{
  y::string_vector paths;
  filesystem.list_pattern(paths, "/tiles/**.png");
  for (const y::string& s : paths) {
    insert_tileset(s, new Tileset(gl.make_texture(s)));
  }

  paths.clear();
  filesystem.list_pattern(paths, "/world/**.cell");
  for (const y::string& s : paths) {
    insert_cell(s, new CellBlueprint());
  }

  paths.clear();
  filesystem.list_pattern(paths, "/world/**.map");
  for (const y::string& s : paths) {
    insert_map(s, new CellMap());
  }

  if (_cell_list.empty() || _map_list.empty()) {
    make_default_map();
  }
}

void Databank::make_default_map()
{
  CellBlueprint* blueprint = new CellBlueprint();
  CellMap* map = new CellMap();
  map->set_coord(CellCoord(0, 0), *blueprint);

  insert_cell("/world/default.cell", blueprint);
  insert_map("/world/default.map", map);
}

void Databank::insert_tileset(const y::string& name, Tileset* tileset)
{
  auto it = _tileset_map.find(name);
  if (it != _tileset_map.end()) {
    it->second = y::move_unique(tileset);
    return;
  }

  _tileset_list.push_back(name);
  std::sort(_tileset_list.begin(), _tileset_list.end());
  _tileset_map.insert(y::make_pair(name, y::move_unique(tileset)));
}

void Databank::insert_cell(const y::string& name, CellBlueprint* blueprint)
{
  auto it = _cell_map.find(name);
  if (it != _cell_map.end()) {
    it->second = y::move_unique(blueprint);
    return;
  }

  _cell_list.push_back(name);
  std::sort(_cell_list.begin(), _cell_list.end());
  _cell_map.insert(y::make_pair(name, y::move_unique(blueprint)));
}

void Databank::insert_map(const y::string& name, CellMap* map)
{
  auto it = _map_map.find(name);
  if (it != _map_map.end()) {
    it->second = y::move_unique(map);
    return;
  }

  _map_list.push_back(name);
  std::sort(_map_list.begin(), _map_list.end());
  _map_map.insert(y::make_pair(name, y::move_unique(map)));

}
