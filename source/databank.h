#ifndef DATABANK_H
#define DATABANK_H

#include "common.h"
#include "cell.h"
#include "tileset.h"

class Filesystem;
class GlUtil;

class Databank : public y::no_copy {
public:

  Databank(const Filesystem& filesystem, GlUtil& gl);

private:

  void make_default_map();

  // Insert tileset (becomes owned by the databank).
  void insert_tileset(const y::string& name, Tileset* tileset);
  // Insert cell (becomes owned by the databank).
  void insert_cell(const y::string& name, CellBlueprint* blueprint);
  // Insert map (becomes owned by the databank).
  void insert_map(const y::string& name, CellMap* map);

  typedef y::map<y::string, y::unique<Tileset>> tileset_map;
  typedef y::map<y::string, y::unique<CellBlueprint>> cell_map;
  typedef y::map<y::string, y::unique<CellMap>> map_map;

  y::string_vector _tileset_list;
  y::string_vector _cell_list;
  y::string_vector _map_list;

  tileset_map _tileset_map;
  cell_map _cell_map;
  map_map _map_map;

};

#endif
