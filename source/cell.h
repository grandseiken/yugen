#ifndef CELL_H
#define CELL_H

#include "common/int_types.h"
#include "common/map.h"
#include "common/no_copy.h"
#include "common/pointer.h"

class CellBlueprint;
class Tileset;

struct Tile {

  Tile(const Tileset* tileset = y::null, y::size index = 0);

  bool operator==(const Tile& tile) const;
  bool operator!=(const Tile& tile) const;

  const Tileset* tileset;
  y::size index;

};

class Cell : public y::no_copy {
public:

  static const y::size cell_width = 40;
  static const y::size cell_height = 30;

  // Number of background and foreground layers, not including the collision
  // layer.
  static const y::size background_layers = 1;
  static const y::size foreground_layers = 1;

  // Layer enumeration. 
  static const y::int8 layer_background = -1;
  static const y::int8 layer_collision  =  0;
  static const y::int8 layer_foreground =  1;

  Cell(const CellBlueprint& blueprint);

  // Get the tile at a given position.
  const Tile& get_tile(y::int8 layer, y::size x, y::size y) const;

  // Set the tile at a given position.
  /****/ void set_tile(y::int8 layer, y::size x, y::size y, const Tile& tile);

  // Returns true iff the tileset is currently used for any tile.
  bool is_tileset_used(const Tileset& tileset) const;

private:

  const CellBlueprint& _blueprint;

  y::map<const Tileset*, y::int32> _changed_tilesets;
  y::map<y::size, Tile> _changed_tiles;

};

class CellBlueprint : public y::no_copy {
public:

  CellBlueprint();

  // Get the tile at a given position.
  const Tile& get_tile(y::int8 layer, y::size x, y::size y) const;

  // Set the tile at a given position.
  /****/ void set_tile(y::int8 layer, y::size x, y::size y, const Tile& tile);

  // Returns true iff the tileset is currently used for any tile.
  bool is_tileset_used(const Tileset& tileset) const;

  // Returns the number of tiles that use the tileset.
  y::size get_tileset_use_count(const Tileset& tileset) const;

private:

  y::map<const Tileset*, y::size> _tilesets; 
  y::unique<Tile[]> _tiles;

};

#endif
