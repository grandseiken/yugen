#include "tileset.h"

#include "../render/gl_util.h"
#include "../../gen/proto/cell.pb.h"

const y::ivec2 Tileset::tile_size{Tileset::tile_width, Tileset::tile_height};
const y::ivec2 Tileset::ul{0, 0};
const y::ivec2 Tileset::ur{tile_size[xx], 0};
const y::ivec2 Tileset::dl{0, tile_size[yy]};
const y::ivec2 Tileset::dr{tile_size[xx], tile_size[yy]};
const y::ivec2 Tileset::u{tile_size[xx] / 2, 0};
const y::ivec2 Tileset::d{tile_size[xx] / 2, tile_size[yy]};
const y::ivec2 Tileset::l{0, tile_size[yy] / 2};
const y::ivec2 Tileset::r{tile_size[xx], tile_size[yy] / 2};

Tileset::Tileset(const Sprite& texture)
  : _size{texture.texture.get_size() / tile_size}
  , _texture(texture)
  , _collision(new Collision[get_tile_count()])
{
  for (y::size i = 0; i < get_tile_count(); ++i) {
    _collision[i] = COLLIDE_NONE;
  }
}

const Sprite& Tileset::get_texture() const
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

Tileset::Collision Tileset::get_collision(y::int32 index) const
{
  return _collision[index];
}

Tileset::Collision Tileset::get_collision(const y::ivec2& coords) const
{
  return _collision[to_index(coords)];
}

void Tileset::set_collision(y::int32 index, Collision collision)
{
  _collision[index] = collision;
}

void Tileset::set_collision(const y::ivec2& coords, Collision collision)
{
  _collision[to_index(coords)] = collision;
}

void Tileset::save_to_proto(const Databank&, proto::Tileset& proto) const
{
  for (y::size i = 0; i < get_tile_count(); ++i) {
    proto.add_collision(_collision[i]);
  }
}

void Tileset::load_from_proto(Databank&, const proto::Tileset& proto)
{
  for (y::int32 i = 0; i < y::int32(get_tile_count()) &&
                       i < proto.collision_size(); ++i) {
    _collision[i] = Collision(proto.collision(i));
  }
}
