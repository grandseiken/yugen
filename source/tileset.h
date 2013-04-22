#ifndef TILESET_H
#define TILESET_H

#include "common.h"
#include "gl_util.h"
#include "vector.h"

class GlTexture;

class Tileset {
public:

  static const y::int32 tile_width = 32;
  static const y::int32 tile_height = 32;

  Tileset(const GlTexture& texture);

  const GlTexture& get_texture() const;
  const y::ivec2& get_size() const;
  y::size get_tile_count() const;

  y::ivec2 from_index(y::int32 index) const;
  y::int32 to_index(const y::ivec2& coords) const;

private:

  GlTexture _texture;

  // Width and height, in tiles.
  y::ivec2 _size;

};

#endif
