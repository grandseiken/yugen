#ifndef TILESET_H
#define TILESET_H

#include "common.h"

class GlTexture;

class Tileset {
public:

  static const y::size tile_width = 32;
  static const y::size tile_height = 32;

  Tileset(const GlTexture& texture);

  y::size get_width() const;
  y::size get_height() const;
  y::size get_tile_count() const;

private:

  const GlTexture& _texture;

  // Width and height, in tiles.
  y::size _width;
  y::size _height;

};

#endif
