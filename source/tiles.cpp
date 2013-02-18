#includes "tiles.h"

Tileset::Tileset(const Texture& texture)
  : _texture(texture)
  , _width(0)
  , _height(0)
{
}

Tileset::~Tileset()
{
}

Tile::Tile(const Tileset& tileset, std::size_t index)
  : _tileset(tileset)
  , _index(index)
{
}

Tile::~Tile()
{
}
