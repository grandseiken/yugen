#include "cell.h"
#include "tileset.h"

CellMap::CellMap()
  : _min(0, 0)
  , _max(0, 0)
{
}

bool CellMap::is_coord_used(const y::ivec2& cell_coord) const
{
  return _map.find(cell_coord) != _map.end();
}

void CellMap::clear_coord(const y::ivec2& cell_coord)
{
  auto it = _map.find(cell_coord);
  if (it != _map.end()) {
    _map.erase(it);
  }
  _boundary_dirty = true;
}

void CellMap::set_coord(const y::ivec2& cell_coord, CellBlueprint& blueprint)
{
  _map[cell_coord] = &blueprint;
  _boundary_dirty = true;
}

const y::ivec2& CellMap::get_boundary_min() const
{
  recalculate_boundary();
  return _min;
}

const y::ivec2& CellMap::get_boundary_max() const
{
  recalculate_boundary();
  return _max;
}

void CellMap::recalculate_boundary() const
{
  if (!_boundary_dirty) {
    return;
  }

  bool first = true;
  for (const auto& pair : _map) {
    if (first) {
      _max = y::ivec2{1, 1} + pair.first;
      _min = pair.first;
    }
    else {
      _max = y::max(_max, y::ivec2{1, 1} + pair.first);
      _min = y::min(_min, pair.first);
    }
    first = false;
  }
  _boundary_dirty = false;
}

const CellBlueprint* CellMap::get_coord(const y::ivec2& cell_coord) const
{
  auto it = _map.find(cell_coord);
  return it != _map.end() ? it->second : y::null;
}

CellBlueprint* CellMap::get_coord(const y::ivec2& cell_coord)
{
  auto it = _map.find(cell_coord);
  return it != _map.end() ? it->second : y::null;
}

Tile::Tile(const Tileset* tileset, y::size index)
  : tileset(tileset)
  , index(index)
{
}

bool Tile::operator==(const Tile& tile) const
{
  return (tileset == 0 && tile.tileset == 0) ||
      (tileset == tile.tileset && index == tile.index);
}

bool Tile::operator!=(const Tile& tile) const
{
  return !operator==(tile);
}

y::size to_internal_index(y::int32 layer, const y::ivec2& v)
{
  return v[yy] * Cell::cell_width + v[xx] +
      (layer + Cell::background_layers) * Cell::cell_width * Cell::cell_height;
}

// Increment or decrement a value in a map, such that an entry exists iff its
// value is nonzero.
template<typename T, typename U>
void change_by_one(y::map<T, U>& map, const T& t, bool increment)
{
  if (!t) {
    return;
  }

  auto it = map.find(t);
  if (it != map.end()) {
    if (increment) {
      ++it->second;
    }
    else {
      --it->second;
    }
    if (!it->second) {
      map.erase(it);
    }
  }
  else if (increment || T(-1) < T(0)) {
    map[t] = increment ? 1 : -1;
  }
}

const y::ivec2 Cell::cell_size{Cell::cell_width, Cell::cell_height};

Cell::Cell(const CellBlueprint& blueprint)
  : _blueprint(blueprint)
{
}

const Tile& Cell::get_tile(y::int32 layer, const y::ivec2& v) const
{
  auto it = _changed_tiles.find(to_internal_index(layer, v));
  if (it != _changed_tiles.end()) {
    return it->second;
  }
  return _blueprint.get_tile(layer, v);
}

void Cell::set_tile(y::int32 layer, const y::ivec2& v, const Tile& tile)
{
  change_by_one(_changed_tilesets, tile.tileset, true);

  y::size internal_index = to_internal_index(layer, v);
  auto it = _changed_tiles.find(internal_index);

  // Keep track of which tilesets are used in the diff.
  if (it != _changed_tiles.end()) {
    change_by_one(_changed_tilesets, it->second.tileset, false);

    if (tile == _blueprint.get_tile(layer, v)) {
      _changed_tiles.erase(it);
      return;
    }
    it->second = tile;
  }
  else {
    change_by_one(_changed_tilesets,
                  _blueprint.get_tile(layer, v).tileset, false);

    _changed_tiles[internal_index] = tile;
  }
}

bool Cell::is_tileset_used(const Tileset& tileset) const
{
  auto it = _changed_tilesets.find(&tileset);
  y::int32 change_diff = it != _changed_tilesets.end() ? it->second : 0;
  return change_diff + _blueprint.get_tileset_use_count(tileset) > 0;
}

CellBlueprint::CellBlueprint()
  : _tiles(new Tile[Cell::cell_width * Cell::cell_height * (
      1 + Cell::foreground_layers + Cell::background_layers)])
{
}

const Tile& CellBlueprint::get_tile(y::int32 layer, const y::ivec2& v) const
{
  return _tiles[to_internal_index(layer, v)];
}

void CellBlueprint::set_tile(y::int32 layer, const y::ivec2& v,
                             const Tile& tile)
{
  y::size internal_index = to_internal_index(layer, v);
  change_by_one(_tilesets, _tiles[internal_index].tileset, false);
  _tiles[internal_index] = tile;
  change_by_one(_tilesets, tile.tileset, true);
}

bool CellBlueprint::is_tileset_used(const Tileset& tileset) const
{
  return get_tileset_use_count(tileset);
}

y::size CellBlueprint::get_tileset_use_count(const Tileset& tileset) const
{
  auto it = _tilesets.find(&tileset);
  return it != _tilesets.end() ? it->second : 0;
}
