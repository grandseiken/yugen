#ifndef TILESET_H
#define TILESET_H

#include "common.h"

class GlTexture;

class Tileset {
public:

  static const y::size tile_width = 32;
  static const y::size tile_height = 32;

  Tileset(const GlTexture& texture);

private:

  const GlTexture& _texture;
  y::size _width;
  y::size _height;

};

#endif
