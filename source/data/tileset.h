#ifndef DATA__TILESET_H
#define DATA__TILESET_H

#include "../common/memory.h"
#include "../render/gl_handle.h"
#include "../save.h"
#include "../vector.h"

#ifndef SPRITE_DEC
#define SPRITE_DEC
struct Sprite {
  GlTexture2D texture;
  GlTexture2D normal;
};
#endif
namespace proto {
  class Tileset;
}

class Tileset : public y::io<proto::Tileset> {
public:

  enum Collision {
    COLLIDE_NONE,
    COLLIDE_FULL,
    COLLIDE_HALF_U,
    COLLIDE_HALF_D,
    COLLIDE_HALF_L,
    COLLIDE_HALF_R,
    COLLIDE_SLOPE1_UL,
    COLLIDE_SLOPE1_UR,
    COLLIDE_SLOPE1_DL,
    COLLIDE_SLOPE1_DR,
    COLLIDE_SLOPE2_UL_A,
    COLLIDE_SLOPE2_UL_B,
    COLLIDE_SLOPE2_UR_A,
    COLLIDE_SLOPE2_UR_B,
    COLLIDE_SLOPE2_DL_A,
    COLLIDE_SLOPE2_DL_B,
    COLLIDE_SLOPE2_DR_A,
    COLLIDE_SLOPE2_DR_B,
    COLLIDE_SLOPEH_UL_A,
    COLLIDE_SLOPEH_UL_B,
    COLLIDE_SLOPEH_UR_A,
    COLLIDE_SLOPEH_UR_B,
    COLLIDE_SLOPEH_DL_A,
    COLLIDE_SLOPEH_DL_B,
    COLLIDE_SLOPEH_DR_A,
    COLLIDE_SLOPEH_DR_B,
    COLLIDE_SIZE,
  };

  static const y::int32 tile_width = 32;
  static const y::int32 tile_height = 32;
  static const y::ivec2 tile_size;
  static const y::ivec2 ul;
  static const y::ivec2 ur;
  static const y::ivec2 dl;
  static const y::ivec2 dr;
  static const y::ivec2 u;
  static const y::ivec2 d;
  static const y::ivec2 l;
  static const y::ivec2 r;

  Tileset(const Sprite& texture);
  const Sprite& get_texture() const;

  const y::ivec2& get_size() const;
  y::size get_tile_count() const;

  y::ivec2 from_index(y::int32 index) const;
  y::int32 to_index(const y::ivec2& coords) const;

  Collision get_collision(y::int32 index) const;
  Collision get_collision(const y::ivec2& coords) const;

  void set_collision(y::int32 index, Collision collision);
  void set_collision(const y::ivec2& coords, Collision collision);

protected:

  void save_to_proto(const Databank& bank,
                     proto::Tileset& proto) const override;
  void load_from_proto(Databank& bank, const proto::Tileset& proto) override;

private:

  // Width and height, in tiles.
  y::ivec2 _size;

  Sprite _texture;
  y::unique<Collision[]> _collision;

};

#endif
