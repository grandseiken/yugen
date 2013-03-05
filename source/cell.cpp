#include "cell.h"
#include "tileset.h"

#include <boost/functional/hash.hpp>

CellCoord::CellCoord(y::int32 x, y::int32 y)
  : x(x)
  , y(y)
{
}

bool CellCoord::operator==(const CellCoord& cell_coord) const
{
  return x == cell_coord.x && y == cell_coord.y;
}

bool CellCoord::operator!=(const CellCoord& cell_coord) const
{
  return !operator==(cell_coord);
}

CellCoord CellCoord::operator+(const CellCoord& cell_coord) const
{
  return CellCoord(x + cell_coord.x, y + cell_coord.y);
}

CellCoord CellCoord::operator-(const CellCoord& cell_coord) const
{
  return CellCoord(x - cell_coord.x, y - cell_coord.y);
}

CellCoord CellCoord::operator-() const
{
  return CellCoord(-x, -y);
}

CellCoord& CellCoord::operator+=(const CellCoord& cell_coord)
{
  return operator=(operator+(cell_coord));
}

CellCoord& CellCoord::operator-=(const CellCoord& cell_coord)
{
  return operator=(operator-(cell_coord));
}

namespace std {

  y::size hash<CellCoord>::operator()(const CellCoord& cell_coord) const
  {
    y::size seed = 0;
    boost::hash_combine(seed, cell_coord.x);
    boost::hash_combine(seed, cell_coord.y);
    return seed;
  }

}

bool CellMap::is_coord_used(const CellCoord& cell_coord) const
{
  return _map.find(cell_coord) != _map.end();
}

void CellMap::clear_coord(const CellCoord& cell_coord)
{
  auto it = _map.find(cell_coord);
  if (it != _map.end()) {
    _map.erase(it);
  }
}

void CellMap::set_coord(const CellCoord& cell_coord, CellBlueprint& blueprint)
{
  _map[cell_coord] = &blueprint;
}

const CellBlueprint* CellMap::get_coord(const CellCoord& cell_coord) const
{
  auto it = _map.find(cell_coord);
  return it != _map.end() ? it->second : y::null;
}

CellBlueprint* CellMap::get_coord(const CellCoord& cell_coord)
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

y::size to_internal_index(y::int8 layer, y::size x, y::size y)
{
  return y * Cell::cell_width + x +
      (layer + Cell::background_layers) * Cell::cell_width * Cell::cell_height;
}

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

Cell::Cell(const CellBlueprint& blueprint)
  : _blueprint(blueprint)
{
}

const Tile& Cell::get_tile(y::int8 layer, y::size x, y::size y) const
{
  auto it = _changed_tiles.find(to_internal_index(layer, x, y));
  if (it != _changed_tiles.end()) {
    return it->second;
  }
  return _blueprint.get_tile(layer, x, y);
}

void Cell::set_tile(y::int8 layer, y::size x, y::size y, const Tile& tile)
{
  change_by_one(_changed_tilesets, tile.tileset, true);

  y::size internal_index = to_internal_index(layer, x, y);
  auto it = _changed_tiles.find(internal_index);

  if (it != _changed_tiles.end()) {
    change_by_one(_changed_tilesets, it->second.tileset, false);

    if (tile == _blueprint.get_tile(layer, x, y)) {
      _changed_tiles.erase(it);
      return;
    }
    it->second = tile;
  }
  else {
    change_by_one(_changed_tilesets,
                  _blueprint.get_tile(layer, x, y).tileset, false);

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

const Tile& CellBlueprint::get_tile(y::int8 layer, y::size x, y::size y) const
{
  return _tiles[to_internal_index(layer, x, y)]; 
}

void CellBlueprint::set_tile(y::int8 layer, y::size x, y::size y,
                             const Tile& tile)
{
  y::size internal_index = to_internal_index(layer, x, y);
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
