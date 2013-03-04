#include "cell_map.h"
#include "cell.h"

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
