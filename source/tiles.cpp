#include "tiles.h"

Tileset::Tileset(const Texture& texture)
  : _texture(texture)
  , _width(0)
  , _height(0)
{
}

Tileset::~Tileset()
{
}

Tile::Tile(const Tileset& tileset, y::size index)
  : _tileset(tileset)
  , _index(index)
{
}

Tile::~Tile()
{
}
