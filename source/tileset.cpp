#include "tileset.h"
#include "gl_util.h"

Tileset::Tileset(const GlTexture& texture)
  : _texture(texture)
  , _width(texture.get_width() / tile_width)
  , _height(texture.get_height() / tile_height)
{
}
