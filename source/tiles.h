#ifndef TILES_H
#define TILES_H

#include "common.h"

class Texture;

class Tileset {
public:

  Tileset(const Texture& texture);
  ~Tileset()

private:

  const Texture& _texture;
  const y::size _width;
  const y::size _height;

};

struct Tile {

  Tile(const Tileset& tileset, y::size index);
  ~Tile();

  const Tileset& _tileset;
  y::size _index;  

};

#endif
