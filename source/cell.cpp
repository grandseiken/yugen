#include "cell.h"
#include "tileset.h"

Tile::Tile(y::size tileset, y::size index)
  : _tileset(tileset)
  , _index(index)
{
}

Tile::~Tile()
{
}

Cell::Cell()
: _tiles(new Tile[cell_width * cell_height])
{
}

Cell::~Cell()
{
}
