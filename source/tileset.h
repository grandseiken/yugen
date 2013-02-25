#ifndef TILESET_H
#define TILESET_H

#include "common.h"

class Texture;

class Tileset {
public:

  static const y::size tile_width = 32;
  static const y::size tile_height = 32;

  Tileset(const Texture& texture);

private:

  const Texture& _texture;
  const y::size _width;
  const y::size _height;

};

#endif
