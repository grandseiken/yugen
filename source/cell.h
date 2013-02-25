#ifndef CELL_H
#define CELL_H

#include "common.h"

class Tileset;

struct Tile {

  static const y::size no_tileset = -1;

  Tile(y::size tileset = no_tileset, y::size index = 0);

  bool operator==(const Tile& tile) const;
  bool operator!=(const Tile& tile) const;

  y::size _tileset;
  y::size _index;

};

class Cell {
public:

  static const y::size cell_width = 40;
  static const y::size cell_height = 30;

  Cell();

private:

  y::vector<Tileset*> _tilesets; 
  y::unique<Tile[]> _tiles;

};

#endif
