#include "cell.h"
#include "tileset.h"

Tile::Tile(y::size tileset, y::size index)
  : _tileset(tileset)
  , _index(index)
{
}

bool Tile::operator==(const Tile& tile) const
{
  return _tileset == tile._tileset && _index == tile._index;
}

bool Tile::operator!=(const Tile& tile) const
{
  return !operator==(tile);
}

Cell::Cell()
: _tiles(new Tile[cell_width * cell_height])
{
}
