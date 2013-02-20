#include "tileset.h"

Tileset::Tileset(const Texture& texture)
  : _texture(texture)
  , _width(0)
  , _height(0)
{
}

Tileset::~Tileset()
{
}
