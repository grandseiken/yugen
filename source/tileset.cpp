#include "tileset.h"
#include "gl_util.h"

Tileset::Tileset(const GlTexture& texture)
  : _texture(texture)
  , _width(texture.get_width() / tile_width)
  , _height(texture.get_height() / tile_height)
{
}

const GlTexture& Tileset::get_texture() const
{
  return _texture;
}

y::size Tileset::get_width() const
{
  return _width;
}

y::size Tileset::get_height() const
{
  return _height;
}

y::size Tileset::get_tile_count() const
{
  return get_width() * get_height();
}
