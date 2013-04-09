#ifndef DATABANK_H
#define DATABANK_H

#include "common.h"
#include "cell.h"
#include "tileset.h"

class Filesystem;
class GlUtil;

// TODO: exceptions?
class Databank : public y::no_copy {
public:

  Databank(const Filesystem& filesystem, GlUtil& gl);

  const y::string_vector& get_tileset_list() const;
  const y::string_vector& get_cell_list() const;
  const y::string_vector& get_map_list() const;

  const Tileset& get_tileset(const y::string& name) const;
  /***/ Tileset& get_tileset(const y::string& name);

  const CellBlueprint& get_cell(const y::string& name) const;
  /***/ CellBlueprint& get_cell(const y::string& name);

  const CellMap& get_map(const y::string& name) const;
  /***/ CellMap& get_map(const y::string& name);

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

  Tileset _default_tileset;
  CellBlueprint _default_cell;
  CellMap _default_map;

};

#endif
