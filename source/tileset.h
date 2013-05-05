#ifndef TILESET_H
#define TILESET_H

#include "common.h"
#include "gl_util.h"
#include "save.h"
#include "vector.h"

class GlTexture;
namespace proto {
  class Tileset;
}

class Tileset : public y::io<proto::Tileset> {
public:

  enum Collision {
    COLLIDE_NONE,
    COLLIDE_FULL,
    COLLIDE_SIZE
  };

  static const y::int32 tile_width = 32;
  static const y::int32 tile_height = 32;
  static const y::ivec2 tile_size;

  Tileset(const GlTexture& texture);

  const GlTexture& get_texture() const;
  const y::ivec2& get_size() const;
  y::size get_tile_count() const;

  y::ivec2 from_index(y::int32 index) const;
  y::int32 to_index(const y::ivec2& coords) const;

  Collision get_collision(y::int32 index) const;
  Collision get_collision(const y::ivec2& coords) const;

  void set_collision(y::int32 index, Collision collision);
  void set_collision(const y::ivec2& coords, Collision collision);

protected:

  virtual void save_to_proto(const Databank& bank, proto::Tileset& proto) const;
  virtual void load_from_proto(Databank& bank, const proto::Tileset& proto);

private:

  // Width and height, in tiles.
  y::ivec2 _size;

  GlTexture _texture;
  y::unique<Collision[]> _collision;

};

#endif
