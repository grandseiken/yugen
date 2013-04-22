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

y::ivec2 Tileset::from_index(y::int32 index) const
{
  return {index % _size[xx], index / _size[xx]};
}

y::int32 Tileset::to_index(const y::ivec2& coords) const
{
  return coords[xx] + coords[yy] * _size[xx];
}
