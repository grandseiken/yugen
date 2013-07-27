#ifndef CELL_H
#define CELL_H

#include "common.h"
#include "save.h"
#include "vector.h"

class CellBlueprint;
class Tileset;
namespace proto {
  class CellMap;
  class CellBlueprint;
}

// A script path with associated region. For simplicity with the common
// case (1x1 script region), the min and max are both inclusive. Measured
// in tiles.
struct ScriptBlueprint {
  y::ivec2 min;
  y::ivec2 max;
  y::string path;
};

// Sparse spatial map of CellBlueprints. CellMaps also store the scripts
// for the whole map.
class CellMap : public y::io<proto::CellMap> {
public:

  CellMap();

  bool is_coord_used(const y::ivec2& cell_coord) const;

  void clear_coord(const y::ivec2& cell_coord);

  void set_coord(const y::ivec2& cell_coord, CellBlueprint& blueprint);

  const CellBlueprint* get_coord(const y::ivec2& cell_coord) const;
  /***/ CellBlueprint* get_coord(const y::ivec2& cell_coord);

  // Get low (inclusive) cell boundary.
  const y::ivec2& get_boundary_min() const;
  // Get high (exclusive) cell boundary.
  const y::ivec2& get_boundary_max() const;
  // Get iterator to iterate over area enclosed by boundary.
  y::ivec2_iterator get_cartesian() const;

  typedef y::vector<ScriptBlueprint> script_list;

  // Script manipulation.
  void add_script(const ScriptBlueprint& blueprint);
  bool has_script(const ScriptBlueprint& blueprint) const;
  void remove_script(const ScriptBlueprint& blueprint);

  const script_list& get_scripts() const;
  bool has_script_at(const y::ivec2& v) const;
  const ScriptBlueprint& get_script_at(const y::ivec2& v) const;

protected:

  void save_to_proto(
      const Databank& bank, proto::CellMap& proto) const override;
  void load_from_proto(Databank& bank, const proto::CellMap& proto) override;

private:

  void recalculate_boundary() const;

  typedef y::map<y::ivec2, CellBlueprint*> blueprint_map;
  blueprint_map _map;

  mutable y::ivec2 _min;
  mutable y::ivec2 _max;
  mutable bool _boundary_dirty;

  script_list _scripts;

};

struct Tile {

  Tile(const Tileset* tileset = y::null, y::size index = 0);

  bool operator==(const Tile& tile) const;
  bool operator!=(const Tile& tile) const;

  y::int32 get_collision() const;

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
  static const y::int32 layer_background = -1;
  static const y::int32 layer_collision = 0;
  static const y::int32 layer_foreground = 1;

  Cell(const CellBlueprint& blueprint);

  // Get the tile at a given position.
  const Tile& get_tile(y::int32 layer, const y::ivec2& v) const;

  // Set the tile at a given position.
  /****/ void set_tile(y::int32 layer, const y::ivec2& v, const Tile& tile);

  // Returns true iff the tileset is currently used for any tile.
  bool is_tileset_used(const Tileset& tileset) const;

private:

  const CellBlueprint& _blueprint;

  y::map<const Tileset*, y::int32> _changed_tilesets;
  y::map<y::size, Tile> _changed_tiles;

};

class CellBlueprint : public y::io<proto::CellBlueprint> {
public:

  static const y::int32 raw_size =
      Cell::cell_width * Cell::cell_height * (
          1 + Cell::foreground_layers + Cell::background_layers);

  CellBlueprint();

  // Get the tile at a given position.
  const Tile& get_tile(y::int32 layer, const y::ivec2& v) const;

  // Set the tile at a given position.
  /****/ void set_tile(y::int32 layer, const y::ivec2& v, const Tile& tile);

  // Returns true iff the tileset is currently used for any tile.
  bool is_tileset_used(const Tileset& tileset) const;

  // Returns the number of tiles that use the tileset.
  y::size get_tileset_use_count(const Tileset& tileset) const;

protected:

  void save_to_proto(const Databank& bank,
                     proto::CellBlueprint& proto) const override;
  void load_from_proto(Databank& bank,
                       const proto::CellBlueprint& proto) override;

private:

  void set_tile_internal(y::size internal_index, const Tile& tile);

  y::map<const Tileset*, y::size> _tilesets;
  y::unique<Tile[]> _tiles;

};

#endif
