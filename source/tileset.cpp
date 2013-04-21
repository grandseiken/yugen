#include "tileset.h"
#include "gl_util.h"

Tileset::Tileset(const GlTexture& texture)
  : _texture(texture)
  , _size{texture.get_size()[xx] / tile_width,
          texture.get_size()[yy] / tile_height}
{
}

const GlTexture& Tileset::get_texture() const
{
  return _texture;
}

const y::ivec2& Tileset::get_size() const
{
  return _size;
}

y::size Tileset::get_tile_count() const
{
  return _size[xx] * _size[yy];
}
