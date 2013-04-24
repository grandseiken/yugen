#ifndef CELL_H
#define CELL_H

#include "common.h"
#include "vector.h"

class CellBlueprint;
class Tileset;

// TODO: replace with y::ivec2?
struct CellCoord {

  CellCoord(y::int32 x, y::int32 y);

  bool operator==(const CellCoord& cell_coord) const;
  bool operator!=(const CellCoord& cell_coord) const;

  CellCoord operator+(const CellCoord& cell_coord) const;
  CellCoord operator-(const CellCoord& cell_coord) const;
  CellCoord operator-() const;

  CellCoord& operator+=(const CellCoord& cell_coord);
  CellCoord& operator-=(const CellCoord& cell_coord);

  y::int32 x;
  y::int32 y;

};

namespace std {

  template<>
  struct hash<CellCoord> {
    y::size operator()(const CellCoord& cell_coord) const;
  };

}

// Sparse spatial map of CellBlueprints.
class CellMap : public y::no_copy {
public:

  CellMap();

  bool is_coord_used(const CellCoord& cell_coord) const;

  void clear_coord(const CellCoord& cell_coord);

  void set_coord(const CellCoord& cell_coord, CellBlueprint& blueprint);

  const CellBlueprint* get_coord(const CellCoord& cell_coord) const;
  /***/ CellBlueprint* get_coord(const CellCoord& cell_coord);

  // Get low (inclusive) cell boundary.
  const CellCoord& get_boundary_min() const;
  // Get high (exclusive) cell boundary.
  const CellCoord& get_boundary_max() const;

private:

  void recalculate_boundary() const;

  typedef y::map<CellCoord, CellBlueprint*> blueprint_map;
  blueprint_map _map;

  mutable CellCoord _min;
  mutable CellCoord _max;
  mutable bool _boundary_dirty;

};

struct Tile {

  Tile(const Tileset* tileset = y::null, y::size index = 0);

  bool operator==(const Tile& tile) const;
  bool operator!=(const Tile& tile) const;

  const Tileset* tileset;
  y::size index;

};

// An instantiation of a CellBlueprint. Modified tiles are stored as a diff
// from the CelLBlueprint. Changes to the underlying blueprint are reflected.
class Cell : public y::no_copy {
public:

  static const y::int32 cell_width = 40;
  static const y::int32 cell_height = 30;
  static const y::ivec2 cell_size;

  // Number of background and foreground layers, not including the collision
  // layer.
  static const y::int32 background_layers = 1;
  static const y::int32 foreground_layers = 1;

  // Layer enumeration.
  static const y::int8 layer_background = -1;
  static const y::int8 layer_collision = 0;
  static const y::int8 layer_foreground = 1;

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
