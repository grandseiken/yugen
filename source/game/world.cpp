#include "world.h"

#include "../data/tileset.h"
#include <boost/functional/hash.hpp>

Geometry::Geometry(const y::ivec2& start, const y::ivec2& end, bool external)
  : start(start)
  , end(end)
  , external(external)
{
}

bool Geometry::operator==(const Geometry& g) const
{
  return start == g.start && end == g.end;
}

bool Geometry::operator!=(const Geometry& g) const
{
  return !operator==(g);
}

namespace std {
  y::size hash<Geometry>::operator()(const Geometry& g) const
  {
    y::size seed = 0;
    boost::hash_combine(seed, g.start[xx]);
    boost::hash_combine(seed, g.start[yy]);
    boost::hash_combine(seed, g.end[xx]);
    boost::hash_combine(seed, g.end[yy]);
    return seed;
  }
}

WorldGeometry::WorldGeometry()
  : _geometry_hash(512)
  , _dirty(false)
{
}

void WorldGeometry::merge_geometry(const CellBlueprint& cell,
                                   const y::ivec2& coord)
{
  clear_geometry(coord);
  calculate_geometry(_buckets[coord], cell);
  _dirty = true;
}

void WorldGeometry::clear_geometry(const y::ivec2& coord)
{
  auto it = _buckets.find(coord);
  if (it != _buckets.end()) {
    _dirty = true;
    _buckets.erase(it);
  }
}

void WorldGeometry::swap_geometry(const y::ivec2& a, const y::ivec2& b)
{
  y::swap(_buckets[a], _buckets[b]);
  _dirty = true;
}

const WorldGeometry::geometry_hash& WorldGeometry::get_geometry() const
{
  if (_dirty) {
    merge_all_geometry();
    _dirty = false;
  }
  return _geometry_hash;
}

void WorldGeometry::calculate_geometry(bucket& bucket,
                                       const CellBlueprint& cell)
{
  static const y::int32 collision_layer = 0;
  enum Edge {
    EDGE_UP,
    EDGE_DOWN,
    EDGE_LEFT,
    EDGE_RIGHT,
    HALF_EDGE_UP_LEFT,
    HALF_EDGE_UP_RIGHT,
    HALF_EDGE_DOWN_LEFT,
    HALF_EDGE_DOWN_RIGHT,
    HALF_EDGE_LEFT_UP,
    HALF_EDGE_LEFT_DOWN,
    HALF_EDGE_RIGHT_UP,
    HALF_EDGE_RIGHT_DOWN,
  };

  // Lots of impenetrable case statements.
  struct local {
    static bool is_tile_blocked(const CellBlueprint& cell,
                                const y::ivec2& v, Edge edge)
    {
      if (!(v >= y::ivec2() && v < Cell::cell_size)) {
        return true;
      }
      y::int32 c = cell.get_tile(collision_layer, v).get_collision();

      switch (edge) {
        case EDGE_UP:
          return
              c == Tileset::COLLIDE_FULL ||
              c == Tileset::COLLIDE_HALF_U ||
              c == Tileset::COLLIDE_SLOPE1_DL ||
              c == Tileset::COLLIDE_SLOPE1_DR ||
              c == Tileset::COLLIDE_SLOPE2_DL_B ||
              c == Tileset::COLLIDE_SLOPE2_DR_B ||
              c == Tileset::COLLIDE_SLOPEH_DL_A ||
              c == Tileset::COLLIDE_SLOPEH_DL_B ||
              c == Tileset::COLLIDE_SLOPEH_DR_A ||
              c == Tileset::COLLIDE_SLOPEH_DR_B;
        case EDGE_DOWN:
          return
              c == Tileset::COLLIDE_FULL ||
              c == Tileset::COLLIDE_HALF_D ||
              c == Tileset::COLLIDE_SLOPE1_UL ||
              c == Tileset::COLLIDE_SLOPE1_UR ||
              c == Tileset::COLLIDE_SLOPE2_UL_B ||
              c == Tileset::COLLIDE_SLOPE2_UR_B ||
              c == Tileset::COLLIDE_SLOPEH_UL_A ||
              c == Tileset::COLLIDE_SLOPEH_UL_B ||
              c == Tileset::COLLIDE_SLOPEH_UR_A ||
              c == Tileset::COLLIDE_SLOPEH_UR_B;
        case EDGE_LEFT:
          return
              c == Tileset::COLLIDE_FULL ||
              c == Tileset::COLLIDE_HALF_L ||
              c == Tileset::COLLIDE_SLOPE1_UL ||
              c == Tileset::COLLIDE_SLOPE1_DL ||
              c == Tileset::COLLIDE_SLOPE2_UL_A ||
              c == Tileset::COLLIDE_SLOPE2_UL_B ||
              c == Tileset::COLLIDE_SLOPE2_DL_A ||
              c == Tileset::COLLIDE_SLOPE2_DL_B ||
              c == Tileset::COLLIDE_SLOPEH_UL_B ||
              c == Tileset::COLLIDE_SLOPEH_DL_B;
        case EDGE_RIGHT:
          return
              c == Tileset::COLLIDE_FULL ||
              c == Tileset::COLLIDE_HALF_R ||
              c == Tileset::COLLIDE_SLOPE1_UR ||
              c == Tileset::COLLIDE_SLOPE1_DR ||
              c == Tileset::COLLIDE_SLOPE2_UR_A ||
              c == Tileset::COLLIDE_SLOPE2_UR_B ||
              c == Tileset::COLLIDE_SLOPE2_DR_A ||
              c == Tileset::COLLIDE_SLOPE2_DR_B ||
              c == Tileset::COLLIDE_SLOPEH_UR_B ||
              c == Tileset::COLLIDE_SLOPEH_DR_B;
        case HALF_EDGE_UP_LEFT:
          return
              is_tile_blocked(cell, v, EDGE_LEFT) ||
              c == Tileset::COLLIDE_HALF_U ||
              c == Tileset::COLLIDE_SLOPEH_DL_A ||
              c == Tileset::COLLIDE_SLOPEH_DR_B;
        case HALF_EDGE_UP_RIGHT:
          return
              is_tile_blocked(cell, v, EDGE_RIGHT) ||
              c == Tileset::COLLIDE_HALF_U ||
              c == Tileset::COLLIDE_SLOPEH_DL_B ||
              c == Tileset::COLLIDE_SLOPEH_DR_A;
        case HALF_EDGE_DOWN_LEFT:
          return
              is_tile_blocked(cell, v, EDGE_LEFT) ||
              c == Tileset::COLLIDE_HALF_D ||
              c == Tileset::COLLIDE_SLOPEH_UL_A ||
              c == Tileset::COLLIDE_SLOPEH_UR_B;
        case HALF_EDGE_DOWN_RIGHT:
          return
              is_tile_blocked(cell, v, EDGE_RIGHT) ||
              c == Tileset::COLLIDE_HALF_D ||
              c == Tileset::COLLIDE_SLOPEH_UL_B ||
              c == Tileset::COLLIDE_SLOPEH_UR_A;
        case HALF_EDGE_LEFT_UP:
          return
              is_tile_blocked(cell, v, EDGE_UP) ||
              c == Tileset::COLLIDE_HALF_L ||
              c == Tileset::COLLIDE_SLOPE2_DL_A ||
              c == Tileset::COLLIDE_SLOPE2_UL_B;
        case HALF_EDGE_LEFT_DOWN:
          return
              is_tile_blocked(cell, v, EDGE_DOWN) ||
              c == Tileset::COLLIDE_HALF_L ||
              c == Tileset::COLLIDE_SLOPE2_DL_B ||
              c == Tileset::COLLIDE_SLOPE2_UL_A;
        case HALF_EDGE_RIGHT_UP:
          return
              is_tile_blocked(cell, v, EDGE_UP) ||
              c == Tileset::COLLIDE_HALF_R ||
              c == Tileset::COLLIDE_SLOPE2_DR_A ||
              c == Tileset::COLLIDE_SLOPE2_UR_B;
        case HALF_EDGE_RIGHT_DOWN:
          return
              is_tile_blocked(cell, v, EDGE_DOWN) ||
              c == Tileset::COLLIDE_HALF_R ||
              c == Tileset::COLLIDE_SLOPE2_DR_B ||
              c == Tileset::COLLIDE_SLOPE2_UR_A;
        default:
          return false;
      };
    }

    // Defines a consistent traversal direction for the edges of irregular
    // tiles.
    static y::ivec2 consistent_traversal(y::int32 collision, bool positive)
    {
      switch (collision) {
        case Tileset::COLLIDE_HALF_U:
        case Tileset::COLLIDE_HALF_D:
          return positive ? y::ivec2{1, 0} : y::ivec2{-1, 0};

        case Tileset::COLLIDE_HALF_L:
        case Tileset::COLLIDE_HALF_R:
          return positive ? y::ivec2{0, 1} : y::ivec2{0, -1};

        case Tileset::COLLIDE_SLOPE1_UL:
        case Tileset::COLLIDE_SLOPE1_DR:
          return positive ? y::ivec2{1, 1} : y::ivec2{-1, -1};

        case Tileset::COLLIDE_SLOPE1_UR:
        case Tileset::COLLIDE_SLOPE1_DL:
          return positive ? y::ivec2{1, -1} : y::ivec2{-1, 1};

        case Tileset::COLLIDE_SLOPEH_UL_A:
        case Tileset::COLLIDE_SLOPEH_DR_B:
          return positive ? y::ivec2{1, 1} : y::ivec2{-1, 0};
        case Tileset::COLLIDE_SLOPEH_UL_B:
        case Tileset::COLLIDE_SLOPEH_DR_A:
          return positive ? y::ivec2{1, 0} : y::ivec2{-1, -1};

        case Tileset::COLLIDE_SLOPEH_UR_A:
        case Tileset::COLLIDE_SLOPEH_DL_B:
          return positive ? y::ivec2{1, 0} : y::ivec2{-1, 1};
        case Tileset::COLLIDE_SLOPEH_UR_B:
        case Tileset::COLLIDE_SLOPEH_DL_A:
          return positive ? y::ivec2{1, -1} : y::ivec2{-1, 0};

        case Tileset::COLLIDE_SLOPE2_UL_A:
        case Tileset::COLLIDE_SLOPE2_DR_B:
          return positive ? y::ivec2{0, 1} : y::ivec2{-1, -1};
        case Tileset::COLLIDE_SLOPE2_UL_B:
        case Tileset::COLLIDE_SLOPE2_DR_A:
          return positive ? y::ivec2{1, 1} : y::ivec2{0, -1};

        case Tileset::COLLIDE_SLOPE2_UR_A:
        case Tileset::COLLIDE_SLOPE2_DL_B:
          return positive ? y::ivec2{0, 1} : y::ivec2{1, -1};
        case Tileset::COLLIDE_SLOPE2_DL_A:
        case Tileset::COLLIDE_SLOPE2_UR_B:
          return positive ? y::ivec2{-1, 1} : y::ivec2{0, -1};

        default:
          return y::ivec2();
      }
    }

    // Given irregular tile, returns the next irregular tile we expect to see
    // in the consistent traversal direction if we are to form a line.
    static y::int32 expected_traversal(y::int32 collision)
    {
      switch (collision) {
        case Tileset::COLLIDE_SLOPE2_UL_A:
          return Tileset::COLLIDE_SLOPE2_UL_B;
        case Tileset::COLLIDE_SLOPE2_UL_B:
          return Tileset::COLLIDE_SLOPE2_UL_A;
        case Tileset::COLLIDE_SLOPE2_UR_A:
          return Tileset::COLLIDE_SLOPE2_UR_B;
        case Tileset::COLLIDE_SLOPE2_UR_B:
          return Tileset::COLLIDE_SLOPE2_UR_A;
        case Tileset::COLLIDE_SLOPE2_DL_A:
          return Tileset::COLLIDE_SLOPE2_DL_B;
        case Tileset::COLLIDE_SLOPE2_DL_B:
          return Tileset::COLLIDE_SLOPE2_DL_A;
        case Tileset::COLLIDE_SLOPE2_DR_A:
          return Tileset::COLLIDE_SLOPE2_DR_B;
        case Tileset::COLLIDE_SLOPE2_DR_B:
          return Tileset::COLLIDE_SLOPE2_DR_A;
        case Tileset::COLLIDE_SLOPEH_UL_A:
          return Tileset::COLLIDE_SLOPEH_UL_B;
        case Tileset::COLLIDE_SLOPEH_UL_B:
          return Tileset::COLLIDE_SLOPEH_UL_A;
        case Tileset::COLLIDE_SLOPEH_UR_A:
          return Tileset::COLLIDE_SLOPEH_UR_B;
        case Tileset::COLLIDE_SLOPEH_UR_B:
          return Tileset::COLLIDE_SLOPEH_UR_A;
        case Tileset::COLLIDE_SLOPEH_DL_A:
          return Tileset::COLLIDE_SLOPEH_DL_B;
        case Tileset::COLLIDE_SLOPEH_DL_B:
          return Tileset::COLLIDE_SLOPEH_DL_A;
        case Tileset::COLLIDE_SLOPEH_DR_A:
          return Tileset::COLLIDE_SLOPEH_DR_B;
        case Tileset::COLLIDE_SLOPEH_DR_B:
          return Tileset::COLLIDE_SLOPEH_DR_A;
        default:
          return collision;
      }
    }

    static void add_traversal_edge(
        geometry_list& list, y::ivec2 min, y::ivec2 max,
        y::int32 min_c, y::int32 max_c)
    {
      min *= Tileset::tile_size;
      max *= Tileset::tile_size;

      switch (min_c) {
        case Tileset::COLLIDE_HALF_U:
          list.emplace_back(max + Tileset::r, min + Tileset::l);
          break;
        case Tileset::COLLIDE_HALF_D:
          list.emplace_back(min + Tileset::l, max + Tileset::r);
          break;
        case Tileset::COLLIDE_HALF_L:
          list.emplace_back(min + Tileset::u, max + Tileset::d);
          break;
        case Tileset::COLLIDE_HALF_R:
          list.emplace_back(max + Tileset::d, min + Tileset::u);
          break;

        case Tileset::COLLIDE_SLOPE1_UL:
          list.emplace_back(min + Tileset::ul, max + Tileset::dr);
          break;
        case Tileset::COLLIDE_SLOPE1_UR:
          list.emplace_back(min + Tileset::dl, max + Tileset::ur);
          break;
        case Tileset::COLLIDE_SLOPE1_DL:
          list.emplace_back(max + Tileset::ur, min + Tileset::dl);
          break;
        case Tileset::COLLIDE_SLOPE1_DR:
          list.emplace_back(max + Tileset::dr, min + Tileset::ul);
          break;

        case Tileset::COLLIDE_SLOPEH_UL_A:
        case Tileset::COLLIDE_SLOPEH_UL_B:
          list.emplace_back(
              min + (min_c == Tileset::COLLIDE_SLOPEH_UL_A ? Tileset::l :
                                                             Tileset::ul),
              max + (max_c == Tileset::COLLIDE_SLOPEH_UL_B ? Tileset::r :
                                                             Tileset::dr));
          break;
        case Tileset::COLLIDE_SLOPEH_UR_A:
        case Tileset::COLLIDE_SLOPEH_UR_B:
          list.emplace_back(
              min + (min_c == Tileset::COLLIDE_SLOPEH_UR_B ? Tileset::l :
                                                             Tileset::dl),
              max + (max_c == Tileset::COLLIDE_SLOPEH_UR_A ? Tileset::r :
                                                             Tileset::ur));
          break;
        case Tileset::COLLIDE_SLOPEH_DL_A:
        case Tileset::COLLIDE_SLOPEH_DL_B:
          list.emplace_back(
              max + (max_c == Tileset::COLLIDE_SLOPEH_DL_B ? Tileset::r :
                                                             Tileset::ur),
              min + (min_c == Tileset::COLLIDE_SLOPEH_DL_A ? Tileset::l :
                                                             Tileset::dl));
          break;
        case Tileset::COLLIDE_SLOPEH_DR_A:
        case Tileset::COLLIDE_SLOPEH_DR_B:
          list.emplace_back(
              max + (max_c == Tileset::COLLIDE_SLOPEH_DR_A ? Tileset::r :
                                                             Tileset::dr),
              min + (min_c == Tileset::COLLIDE_SLOPEH_DR_B ? Tileset::l :
                                                             Tileset::ul));
          break;
        case Tileset::COLLIDE_SLOPE2_UL_A:
        case Tileset::COLLIDE_SLOPE2_UL_B:
          list.emplace_back(
              min + (min_c == Tileset::COLLIDE_SLOPE2_UL_B ? Tileset::u :
                                                             Tileset::ul),
              max + (max_c == Tileset::COLLIDE_SLOPE2_UL_A ? Tileset::d :
                                                             Tileset::dr));
          break;
        case Tileset::COLLIDE_SLOPE2_UR_A:
        case Tileset::COLLIDE_SLOPE2_UR_B:
          list.emplace_back(
              min + (min_c == Tileset::COLLIDE_SLOPE2_UR_A ? Tileset::d :
                                                             Tileset::dl),
              max + (max_c == Tileset::COLLIDE_SLOPE2_UR_B ? Tileset::u :
                                                             Tileset::ur));
          break;
        case Tileset::COLLIDE_SLOPE2_DL_A:
        case Tileset::COLLIDE_SLOPE2_DL_B:
          list.emplace_back(
              min + (min_c == Tileset::COLLIDE_SLOPE2_DL_A ? Tileset::u :
                                                             Tileset::ur),
              max + (max_c == Tileset::COLLIDE_SLOPE2_DL_B ? Tileset::d :
                                                             Tileset::dl));
          break;
        case Tileset::COLLIDE_SLOPE2_DR_A:
        case Tileset::COLLIDE_SLOPE2_DR_B:
          list.emplace_back(
              max + (max_c == Tileset::COLLIDE_SLOPE2_DR_B ? Tileset::d :
                                                             Tileset::dr),
              min + (min_c == Tileset::COLLIDE_SLOPE2_DR_A ? Tileset::u :
                                                             Tileset::ul));
          break;

        default: {}
      }
    }
  };

  // Strategy: treat horizontal and vertical edges completely separately. Loop
  // over the line segments forming the edges of tiles, adding geometry when the
  // collision of the tiles on either side of the edge differs. When this
  // pattern continues for several tiles, don't add the geometry until we reach
  // the end.
  // The same technique could be used for every possible sloped gradient, but
  // since these are probably less common and have the nice property of at most
  // one sloped edge per tile, independent of adjacent tiles, they are handled
  // separately afterwards.
  enum Boundary {
    NONE,
    LEFT,
    RIGHT,
  };
  Boundary boundary;
  y::int32 boundary_start = 0;

  // Horizontal geometry lines.
  for (y::int32 row = 0; row <= Cell::cell_height; ++row) {
    geometry_list& list =
        row == 0 ? bucket.top :
        row == Cell::cell_height ? bucket.bottom : bucket.middle;

    boundary = NONE;
    for (y::int32 t = 0; t <= 2 * Cell::cell_width; ++t) {
      bool above = local::is_tile_blocked(
          cell, {t / 2, row - 1}, t % 2 ? HALF_EDGE_RIGHT_DOWN :
                                          HALF_EDGE_LEFT_DOWN);
      bool below = local::is_tile_blocked(
          cell, {t / 2, row}, t % 2 ? HALF_EDGE_RIGHT_UP :
                                      HALF_EDGE_LEFT_UP);

      Boundary new_boundary = above && !below ? LEFT :
                              below && !above ? RIGHT : NONE;
      if (boundary == new_boundary) {
        continue;
      }

      if (boundary != NONE) {
        y::ivec2 mul = Tileset::tile_size / y::ivec2{2, 1};
        y::ivec2 start = mul * y::ivec2{boundary_start, row};
        y::ivec2 end = mul * y::ivec2{t, row};

        // Make sure the geometry is always on the right of its boundary.
        list.emplace_back(boundary == RIGHT ?
                          Geometry(start, end) : Geometry(end, start));
      }
      if (new_boundary != NONE) {
        boundary_start = t;
      }
      boundary = new_boundary;
    }
  }

  // Vertical geometry lines.
  for (y::int32 col = 0; col <= Cell::cell_width; ++col) {
    geometry_list& list =
        col == 0 ? bucket.left :
        col == Cell::cell_width ? bucket.right : bucket.middle;

    boundary = NONE;
    for (y::int32 t = 0; t <= 2 * Cell::cell_height; ++t) {
      bool left = local::is_tile_blocked(
          cell, {col - 1, t / 2}, t % 2 ? HALF_EDGE_DOWN_RIGHT :
                                          HALF_EDGE_UP_RIGHT);
      bool right = local::is_tile_blocked(
          cell, {col, t / 2}, t % 2 ? HALF_EDGE_DOWN_LEFT :
                                      HALF_EDGE_UP_LEFT);

      // Since we're moving downwards, left tile blocked means
      // boundary is actually on the right.
      Boundary new_boundary = left && !right ? RIGHT :
                              right && !left ? LEFT : NONE;
      if (boundary == new_boundary) {
        continue;
      }

      if (boundary != NONE) {
        y::ivec2 mul = Tileset::tile_size / y::ivec2{1, 2};
        y::ivec2 start = mul * y::ivec2{col, boundary_start};
        y::ivec2 end = mul * y::ivec2{col, t};

        list.emplace_back(boundary == RIGHT ?
                          Geometry(start, end) : Geometry(end, start));
      }
      if (new_boundary != NONE) {
        boundary_start = t;
      }
      boundary = new_boundary;
    }
  }

  // Full straight edges of irregular tiles are handled by the above strategy.
  // Now we make a list of non-full tiles so we can go back and fill in the
  // sloped edges.
  y::set<y::ivec2> set;
  for (auto it = y::cartesian(Cell::cell_size); it; ++it) {
    y::int32 c = cell.get_tile(collision_layer, *it).get_collision();
    if (c != Tileset::COLLIDE_NONE && c != Tileset::COLLIDE_FULL) {
      set.insert(*it);
    }
  }

  // Now pick one irregular tile at a time and find the longest line formed
  // by its sloped edge. Erase the tiles we've used so that we don't make
  // parts of the same line multiple times.
  while (!set.empty()) {
    y::ivec2 v = *set.begin();
    set.erase(v);
    y::int32 collision = cell.get_tile(collision_layer, v).get_collision();

    // Scan all the way in both directions to the end of the sloped edge.
    y::int32 c = collision;
    y::ivec2 u = v;
    y::ivec2 dir = local::consistent_traversal(c, true);
    y::int32 next = cell.get_tile(collision_layer, u + dir).get_collision();

    while (next == local::expected_traversal(c)) {
      u += dir;
      set.erase(u);

      c = next;
      dir = local::consistent_traversal(c, true);
      next = cell.get_tile(collision_layer, u + dir).get_collision();
    }
    y::ivec2 max = u;
    y::int32 max_c = c;

    // Other direction.
    c = collision;
    u = v;
    dir = local::consistent_traversal(c, false);
    next = cell.get_tile(collision_layer, u + dir).get_collision();

    while (next == local::expected_traversal(c)) {
      u += dir;
      set.erase(u);

      c = next;
      dir = local::consistent_traversal(c, false);
      next = cell.get_tile(collision_layer, u + dir).get_collision();
    }
    y::ivec2 min = u;
    y::int32 min_c = c;

    // Add the line.
    local::add_traversal_edge(bucket.middle, min, max, min_c, max_c);
  }
}

void WorldGeometry::merge_all_geometry() const
{
  y::ivec2 min;
  y::ivec2 max;
  bool first = true;
  for (const auto& pair : _buckets) {
    if (first) {
      min = pair.first;
      max = pair.first + y::ivec2{1, 1};
    }
    else {
      min = y::min(min, pair.first);
      max = y::max(max, pair.first + y::ivec2{1, 1});
    }
    first = false;
  }

  struct local {
    static void insert(geometry_hash& target, const Geometry& g)
    {
      target.update(g, y::wvec2(y::min(g.start, g.end)),
                       y::wvec2(y::max(g.start, g.end)));
    }

    static void insert(geometry_hash& target,
                       const geometry_list& list, const y::ivec2& offset,
                       bool external)
    {
      for (const Geometry& g : list) {
        insert(target, Geometry(g.start + offset, g.end + offset, external));
      }
    }

    static void merge_loop(
        geometry_hash& target,
        const y::ivec2& a_offset, const y::ivec2& b_offset,
        y::int32 a_min, y::int32 a_max, y::int32 b_min, y::int32 b_max,
        y::size& a_index, y::size& b_index, geometry_list& a, geometry_list& b)
    {
      if (a_max < b_min) {
        insert(target, Geometry(a_offset + a[a_index].start,
                                a_offset + a[a_index].end, true));
        ++a_index;
        return;
      }
      if (b_max < a_min) {
        insert(target, Geometry(b_offset + b[b_index].start,
                                b_offset + b[b_index].end, true));
        ++b_index;
        return;
      }

      // This rest magically works for all cases because the one of the
      // lists has segments stored in reverse.
      if (a_min != b_min) {
        insert(target, Geometry(a_offset + a[a_index].start,
                                b_offset + b[b_index].end, true));
      }

      if (a_max < b_max) {
        b[b_index].end = a[a_index].end - b_offset + a_offset;
        ++a_index;
      }
      else if (a_max > b_max) {
        a[a_index].start = b[b_index].start - a_offset + b_offset;
        ++b_index;
      }
      else {
        ++a_index;
        ++b_index;
      }
    }
  };

  _geometry_hash.clear();
  for (auto it = y::cartesian(min, max); it; ++it) {
    auto jt = _buckets.find(*it);
    if (jt == _buckets.end()) {
      continue;
    }

    // Add all non-edge geometry.
    const bucket& bucket = jt->second;
    const y::ivec2 offset = *it * Tileset::tile_size * Cell::cell_size;
    local::insert(_geometry_hash, bucket.middle, offset, false);

    // Where there's no adjacent cell, add the edge geometry.
    if (_buckets.find(*it - y::ivec2{0, 1}) == _buckets.end()) {
      local::insert(_geometry_hash, bucket.top, offset, true);
    }
    if (_buckets.find(*it - y::ivec2{1, 0}) == _buckets.end()) {
      local::insert(_geometry_hash, bucket.left, offset, true);
    }
    if (_buckets.find(*it + y::ivec2{0, 1}) == _buckets.end()) {
      local::insert(_geometry_hash, bucket.bottom, offset, true);
    }
    if (_buckets.find(*it + y::ivec2{1, 0}) == _buckets.end()) {
      local::insert(_geometry_hash, bucket.right, offset, true);
    }

    // Merge edge geometry with adjacent cells. This depends on implementation
    // details of calculate_geometry, in particular that edges are stored in
    // order from left to right or top to bottom.
    jt = _buckets.find(*it + y::ivec2{0, 1});
    if (jt != _buckets.end()) {
      // Make a copy of each list.
      geometry_list top = bucket.bottom;
      geometry_list bottom = jt->second.top;

      y::size top_index = 0;
      y::size bottom_index = 0;
      y::ivec2 bottom_offset =
          (*it + y::ivec2{0, 1}) * Tileset::tile_size * Cell::cell_size;

      while (top_index < top.size() && bottom_index < bottom.size()) {
        y::int32 top_min = top[top_index].start[xx];
        y::int32 top_max = top[top_index].end[xx];

        y::int32 bottom_min = bottom[bottom_index].end[xx];
        y::int32 bottom_max = bottom[bottom_index].start[xx];

        local::merge_loop(_geometry_hash, offset, bottom_offset,
                          top_min, top_max, bottom_min, bottom_max,
                          top_index, bottom_index, top, bottom);
      }
      for (; top_index < top.size(); ++top_index) {
        local::insert(_geometry_hash,
                      Geometry(offset + top[top_index].start,
                               offset + top[top_index].end, true));
      }
      for (; bottom_index < bottom.size(); ++bottom_index) {
        local::insert(_geometry_hash,
                      Geometry(bottom_offset + bottom[bottom_index].start,
                               bottom_offset + bottom[bottom_index].end, true));
      }
    }
    jt = _buckets.find(*it + y::ivec2{1, 0});
    if (jt != _buckets.end()) {
      geometry_list left = bucket.right;
      geometry_list right = jt->second.left;

      y::size left_index = 0;
      y::size right_index = 0;
      y::ivec2 right_offset =
          (*it + y::ivec2{1, 0}) * Tileset::tile_size * Cell::cell_size;

      while (left_index < left.size() && right_index < right.size()) {
        y::int32 left_min = left[left_index].end[yy];
        y::int32 left_max = left[left_index].start[yy];

        y::int32 right_min = right[right_index].start[yy];
        y::int32 right_max = right[right_index].end[yy];

        // This time the right list is the one with reversed segments, so need
        // to pass everything the other way around.
        local::merge_loop(_geometry_hash, right_offset, offset,
                          right_min, right_max, left_min, left_max,
                          right_index, left_index, right, left);
      }
      for (; left_index < left.size(); ++left_index) {
        local::insert(_geometry_hash,
                      Geometry(offset + left[left_index].start,
                               offset + left[left_index].end, true));
      }
      for (; right_index < right.size(); ++right_index) {
        local::insert(_geometry_hash,
                      Geometry(right_offset + right[right_index].start,
                               right_offset + right[right_index].end, true));
      }
    }
  }
}

WorldSource::WorldSource(y::size type_id)
  : _type_id(type_id)
{
}

bool WorldSource::operator!=(const WorldSource& source) const
{
  return !operator==(source);
}

bool WorldSource::types_equal(const WorldSource& source) const
{
  return _type_id == source._type_id;
}

y::size WorldSource::get_type_id() const
{
  return _type_id;
}

CellMapSource::CellMapSource(const CellMap& map)
  : WorldSource(type_id)
  , _map(map)
{
}

bool CellMapSource::operator==(const WorldSource& source) const
{
  return types_equal(source) && &_map == &((CellMapSource*)&source)->_map;
}

void CellMapSource::hash_combine(y::size& seed) const
{
  boost::hash_combine(seed, get_type_id());
  boost::hash_combine(seed, &_map);
}

const CellBlueprint* CellMapSource::get_coord(const y::ivec2& coord) const
{
  return _map.get_coord(coord);
}

const CellMap::script_list& CellMapSource::get_scripts() const
{
  return _map.get_scripts();
}

WorldWindow::WorldWindow(const WorldSource& active_source,
                         const y::ivec2& active_coord)
  : _active_source(&active_source)
  , _active_source_offset(-active_coord)
  , _active_window(new active_window_entry[
      (1 + 2 * active_window_half_size) * (1 + 2 * active_window_half_size)])
{
  update_active_window();
}

void WorldWindow::set_active_source(const WorldSource& active_source,
                                    const y::ivec2& active_coord)
{
  if (_active_source == &active_source) {
    set_active_coord(active_coord);
    return;
  }
  _active_source = &active_source;
  _active_source_offset = -active_coord;
  update_active_window();
}

void WorldWindow::set_active_coord(const y::ivec2& active_coord)
{
  move_active_window(_active_source_offset + active_coord);
}

y::ivec2 WorldWindow::get_active_coord() const
{
  return -_active_source_offset;
}

void WorldWindow::move_active_window(const y::ivec2& offset)
{
  if (offset == y::ivec2()) {
    return;
  }
  y::int32 half_size = active_window_half_size;
  y::ivec2 half_v{half_size, half_size};
  _active_source_offset -= offset;

  // Temporary copy of the active window.
  y::unique<active_window_entry[]> copy(
      new active_window_entry[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // Stores which cells in the copy have been used.
  y::unique<bool[]> used(new bool[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // Stores which cells in the new active window have been filled.
  y::unique<bool[]> done(new bool[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // We swap the geometry by offsetting it. This is fucking retarded.
  y::ivec2 geometry_swap{1 + 2 * active_window_half_size, 0};

  // Copy the active window into the temporary copy.
  for (auto it = get_cartesian(); it; ++it) {
    _active_geometry.swap_geometry(*it, *it + geometry_swap);
    y::size internal_index = to_internal_index(*it);
    copy[internal_index].blueprint = _active_window[internal_index].blueprint;
    copy[internal_index].cell.swap(_active_window[internal_index].cell);
    used[internal_index] = done[internal_index] = false;
  }

  // Move the cells that are still in view from the copy to the new window.
  for (auto it = get_cartesian(); it; ++it) {
    y::ivec2 source = *it + offset;
    y::size internal_index = to_internal_index(*it);
    if (source >= -half_v && source <= half_v) {
      _active_geometry.swap_geometry(source + geometry_swap, *it);
      y::size copy_index = to_internal_index(source);
      _active_window[internal_index].blueprint = copy[copy_index].blueprint;
      _active_window[internal_index].cell.swap(copy[copy_index].cell);
      used[copy_index] = done[internal_index] = true;
    }
  }

  // Create or swap the remaining unfilled cells in the active window.
  for (auto it = get_cartesian(); it; ++it) {
    y::size internal_index = to_internal_index(*it);
    if (done[internal_index]) {
      continue;
    }
    // These all need to be refreshed.
    _refreshed_cells.emplace_back(*it);
    const CellBlueprint* new_blueprint = active_window_target(*it);
    _active_window[internal_index].blueprint = new_blueprint;

    // If the cell happens to be unchanged, swap it in.
    if (!used[internal_index] &&
        copy[internal_index].blueprint == new_blueprint) {
      _active_geometry.swap_geometry(*it + geometry_swap, *it);
      _active_window[internal_index].cell.swap(copy[internal_index].cell);
      continue;
    }

    // Otherwise create it.
    Cell* new_cell = new_blueprint ? new Cell(*new_blueprint) : y::null;
    _active_window[internal_index].cell = y::move_unique(new_cell);
    if (new_blueprint) {
      _active_geometry.merge_geometry(*new_blueprint, *it);
    }
    else {
      _active_geometry.clear_geometry(*it);
    }
    _active_geometry.clear_geometry(*it + geometry_swap);
  }
}

const CellBlueprint* WorldWindow::get_active_window_blueprint(
    const y::ivec2& v) const
{
  return _active_window[to_internal_index(v)].blueprint;
}

const CellBlueprint* WorldWindow::get_active_window_blueprint() const
{
  return get_active_window_blueprint(y::ivec2());
}

Cell* WorldWindow::get_active_window_cell(const y::ivec2& v) const
{
  return _active_window[to_internal_index(v)].cell.get();
}

Cell* WorldWindow::get_active_window_cell() const
{
  return get_active_window_cell(y::ivec2());
}

y::ivec2_iterator WorldWindow::get_cartesian() const
{
  return y::cartesian(
      y::ivec2{-active_window_half_size, -active_window_half_size},
      y::ivec2{1 + active_window_half_size, 1 + active_window_half_size});
}

const WorldGeometry::geometry_hash& WorldWindow::get_geometry() const
{
  return _active_geometry.get_geometry();
}

const WorldWindow::cell_list& WorldWindow::get_refreshed_cells() const
{
  return _refreshed_cells;
}

void WorldWindow::clear_refreshed_cells()
{
  _refreshed_cells.clear();
}

WorldScript WorldWindow::script_blueprint_to_world_script(
    const ScriptBlueprint& blueprint) const
{
  return {
    blueprint.path,
    y::wvec2(Cell::cell_size * Tileset::tile_size * _active_source_offset +
             (Tileset::tile_size * (y::ivec2{1, 1} + blueprint.min +
                                                     blueprint.max)) / 2),
    y::wvec2((Tileset::tile_size * (y::ivec2{1, 1} + blueprint.max -
                                                     blueprint.min)))};
}

y::size WorldWindow::to_internal_index(const y::ivec2& active_window)
{
  const y::int32 half_size = active_window_half_size;
  y::int32 x = active_window[xx] + half_size;
  y::int32 y = active_window[yy] + half_size;
  return y * (1 + 2 * half_size) + x;
}

const CellBlueprint* WorldWindow::active_window_target(
    const y::ivec2& active_window) const
{
  return _active_source->get_coord(active_window - _active_source_offset);
}

void WorldWindow::update_active_window()
{
  for (auto it = get_cartesian(); it; ++it) {
    update_active_window_cell(*it);
  }
}

void WorldWindow::update_active_window_cell(const y::ivec2& v)
{
  y::size internal_index = to_internal_index(v);

  const CellBlueprint* old_blueprint = _active_window[internal_index].blueprint;
  const CellBlueprint* new_blueprint = active_window_target(v);

  // Changed cells or missing cells need to have their scripts refreshed.
  if (old_blueprint != new_blueprint || !new_blueprint) {
    _refreshed_cells.emplace_back(v);
  }

  if (old_blueprint != new_blueprint) {
    Cell* new_cell = new_blueprint ? new Cell(*new_blueprint) : y::null;

    _active_window[internal_index].cell = y::move_unique(new_cell);
    _active_window[internal_index].blueprint = new_blueprint;
    _active_geometry.clear_geometry(v);
    if (new_blueprint) {
      _active_geometry.merge_geometry(*new_blueprint, v);
    }
  }
}

WorldWindow::active_window_entry::active_window_entry()
  : blueprint(y::null)
  , cell(y::null)
{
}

