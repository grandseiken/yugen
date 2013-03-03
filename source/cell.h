#ifndef CELL_H
#define CELL_H

#include "common.h"

class CellBlueprint;
class Tileset;

struct Tile {

  Tile(const Tileset* tileset = y::null, y::size index = 0);

  bool operator==(const Tile& tile) const;
  bool operator!=(const Tile& tile) const;

  const Tileset* tileset;
  y::size index;

};

class Cell {
public:

  static const y::size cell_width = 40;
  static const y::size cell_height = 30;

  static const y::size background_layers = 1;
  static const y::size foreground_layers = 1;

  static const y::int8 layer_background = -1;
  static const y::int8 layer_main = 0;
  static const y::int8 layer_foreground = 1;

  Cell(const CellBlueprint& blueprint);

  const Tile& get_tile(y::int8 layer, y::size x, y::size y) const;
  /****/ void set_tile(y::int8 layer, y::size x, y::size y, const Tile& tile);

  bool is_tileset_used(const Tileset& tileset) const;

private:

  const CellBlueprint& _blueprint;

  y::map<const Tileset*, y::int32> _changed_tilesets;
  y::map<y::size, Tile> _changed_tiles;

};

class CellBlueprint {
public:

  CellBlueprint();

  const Tile& get_tile(y::int8 layer, y::size x, y::size y) const;
  /****/ void set_tile(y::int8 layer, y::size x, y::size y, const Tile& tile);

  bool is_tileset_used(const Tileset& tileset) const;
  y::size get_tileset_use_count(const Tileset& tileset) const;

private:

  y::map<const Tileset*, y::size> _tilesets; 
  y::unique<Tile[]> _tiles;

};

#endif
