#ifndef DATABANK_H
#define DATABANK_H

#include "common.h"
#include "cell.h"
#include "tileset.h"

class Filesystem;
class GlUtil;
class GlTexture;

// Globs and loads resources. Requesting resources by invalid names returns
// a reference to a standard missing resource.
// TODO: exceptions?
class Databank : public y::no_copy {
public:

  Databank(const Filesystem& filesystem, GlUtil& gl);

  // Get list of tileset names.
  const y::string_vector& get_tilesets() const;
  // Get list of cell names.
  const y::string_vector& get_cells() const;
  // Get list of map names.
  const y::string_vector& get_maps() const;

  const Tileset& get_tileset(const y::string& name) const;
  /***/ Tileset& get_tileset(const y::string& name);

  const CellBlueprint& get_cell(const y::string& name) const;
  /***/ CellBlueprint& get_cell(const y::string& name);

  const CellMap& get_map(const y::string& name) const;
  /***/ CellMap& get_map(const y::string& name);

  const Tileset& get_tileset(y::size index) const;
  /***/ Tileset& get_tileset(y::size index);

  const CellBlueprint& get_cell(y::size index) const;
  /***/ CellBlueprint& get_cell(y::size index);

  const CellMap& get_map(y::size index) const;
  /***/ CellMap& get_map(y::size index);

  // Get the names that resources are stored as.
  const y::string& get_tileset_name(Tileset& tileset) const;
  const y::string& get_cell_name(CellBlueprint& cell) const;
  const y::string& get_map_name(CellMap& map) const;

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
