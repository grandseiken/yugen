#include "world.h"

#include "tileset.h"

bool Geometry::order::operator()(const Geometry& a, const Geometry& b) const
{
  y::ivec2 a_min = y::min(a.start, a.end);
  y::ivec2 a_max = y::max(a.start, a.end);
  y::ivec2 b_min = y::min(b.start, b.end);
  y::ivec2 b_max = y::max(b.start, b.end);

  return a_min[xx] < b_min[xx] ? true :
         a_min[xx] > b_min[xx] ? false :
         a_max[xx] < b_max[xx] ? true :
         a_max[xx] > b_max[xx] ? false :
         a_min[yy] < b_min[yy] ? true :
         a_min[yy] > b_min[yy] ? false :
         a_max[yy] < b_max[yy];
}

OrderedGeometry::OrderedGeometry()
  : buckets(1 + 2 * WorldWindow::active_window_half_size)
{
}

void OrderedGeometry::insert(const Geometry& g)
{
  y::int32 max = y::max(g.start[xx], g.end[xx]);
  y::int32 bucket =
      WorldWindow::active_window_half_size +
      y::euclidean_div(max, Cell::cell_size[xx] * Tileset::tile_size[xx]);
  y::clamp<y::int32>(bucket, 0, buckets.size());
  buckets[bucket].insert(g);
}

void OrderedGeometry::clear()
{
  for (auto& bucket : buckets) {
    bucket.clear();
  }
}

y::int32 OrderedGeometry::get_max_for_bucket(y::size index) const
{
  return (1 + index - WorldWindow::active_window_half_size) *
      Cell::cell_size[xx] * Tileset::tile_size[xx];
}

WorldGeometry::WorldGeometry()
  : _dirty(false)
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
  std::swap(_buckets[a], _buckets[b]);
  _dirty = true;
}

const OrderedGeometry& WorldGeometry::get_geometry() const
{
  if (_dirty) {
    merge_all_geometry();
    _dirty = false;
  }
  return _ordered_geometry;
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
        default:
          return false;
      };
    }

    static void add_straight_half_edges(
        geometry_list& list,
        const CellBlueprint& cell, const y::ivec2& v, y::int32 collision)
    {
      static const y::ivec2& t = Tileset::tile_size;
      const y::int32 c = collision;

      // Up-left.
      if ((c == Tileset::COLLIDE_HALF_U || c == Tileset::COLLIDE_SLOPEH_DR_B ||
           c == Tileset::COLLIDE_SLOPEH_DL_A) &&
          !is_tile_blocked(cell, v + y::ivec2{-1, 0}, EDGE_RIGHT)) {
        list.emplace_back(Geometry{v * t + Tileset::l, v * t + Tileset::ul});
      }
      // Up-right.
      if ((c == Tileset::COLLIDE_HALF_U || c == Tileset::COLLIDE_SLOPEH_DL_B ||
           c == Tileset::COLLIDE_SLOPEH_DR_A) &&
          !is_tile_blocked(cell, v + y::ivec2{1, 0}, EDGE_LEFT)) {
        list.emplace_back(Geometry{v * t + Tileset::ur, v * t + Tileset::r});
      }
      // Down-left.
      if ((c == Tileset::COLLIDE_HALF_D || c == Tileset::COLLIDE_SLOPEH_UR_B ||
           c == Tileset::COLLIDE_SLOPEH_UL_A) &&
          !is_tile_blocked(cell, v + y::ivec2{-1, 0}, EDGE_RIGHT)) {
        list.emplace_back(Geometry{v * t + Tileset::dl, v * t + Tileset::l});
      }
      // Down-right.
      if ((c == Tileset::COLLIDE_HALF_D || c == Tileset::COLLIDE_SLOPEH_UL_B ||
           c == Tileset::COLLIDE_SLOPEH_UR_A) &&
          !is_tile_blocked(cell, v + y::ivec2{1, 0}, EDGE_LEFT)) {
        list.emplace_back(Geometry{v * t + Tileset::r, v * t + Tileset::dr});
      }
      // Left-up.
      if ((c == Tileset::COLLIDE_HALF_L || c == Tileset::COLLIDE_SLOPE2_UL_B ||
           c == Tileset::COLLIDE_SLOPE2_DL_A) &&
          !is_tile_blocked(cell, v + y::ivec2{0, -1}, EDGE_DOWN)) {
        list.emplace_back(Geometry{v * t + Tileset::ul, v * t + Tileset::u});
      }
      // Left-down.
      if ((c == Tileset::COLLIDE_HALF_L || c == Tileset::COLLIDE_SLOPE2_DL_B ||
           c == Tileset::COLLIDE_SLOPE2_UL_A) &&
          !is_tile_blocked(cell, v + y::ivec2{0, 1}, EDGE_UP)) {
        list.emplace_back(Geometry{v * t + Tileset::d, v * t + Tileset::dl});
      }
      // Right-up.
      if ((c == Tileset::COLLIDE_HALF_R || c == Tileset::COLLIDE_SLOPE2_UR_B ||
           c == Tileset::COLLIDE_SLOPE2_DR_A) &&
          !is_tile_blocked(cell, v + y::ivec2{0, -1}, EDGE_DOWN)) {
        list.emplace_back(Geometry{v * t + Tileset::u, v * t + Tileset::ur});
      }
      // Right-down.
      if ((c == Tileset::COLLIDE_HALF_R || c == Tileset::COLLIDE_SLOPE2_DR_B ||
           c == Tileset::COLLIDE_SLOPE2_UR_A) &&
          !is_tile_blocked(cell, v + y::ivec2{0, 1}, EDGE_UP)) {
        list.emplace_back(Geometry{v * t + Tileset::dr, v * t + Tileset::d});
      }
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
          list.emplace_back(Geometry{max + Tileset::r, min + Tileset::l});
          break;
        case Tileset::COLLIDE_HALF_D:
          list.emplace_back(Geometry{min + Tileset::l, max + Tileset::r});
          break;
        case Tileset::COLLIDE_HALF_L:
          list.emplace_back(Geometry{min + Tileset::u, max + Tileset::d});
          break;
        case Tileset::COLLIDE_HALF_R:
          list.emplace_back(Geometry{max + Tileset::d, min + Tileset::u});
          break;

        case Tileset::COLLIDE_SLOPE1_UL:
          list.emplace_back(Geometry{min + Tileset::ul, max + Tileset::dr});
          break;
        case Tileset::COLLIDE_SLOPE1_UR:
          list.emplace_back(Geometry{min + Tileset::dl, max + Tileset::ur});
          break;
        case Tileset::COLLIDE_SLOPE1_DL:
          list.emplace_back(Geometry{max + Tileset::ur, min + Tileset::dl});
          break;
        case Tileset::COLLIDE_SLOPE1_DR:
          list.emplace_back(Geometry{max + Tileset::dr, min + Tileset::ul});
          break;

        case Tileset::COLLIDE_SLOPEH_UL_A:
        case Tileset::COLLIDE_SLOPEH_UL_B:
          list.emplace_back(Geometry{
              min + (min_c == Tileset::COLLIDE_SLOPEH_UL_A ? Tileset::l :
                                                             Tileset::ul),
              max + (max_c == Tileset::COLLIDE_SLOPEH_UL_B ? Tileset::r :
                                                             Tileset::dr)});
          break;
        case Tileset::COLLIDE_SLOPEH_UR_A:
        case Tileset::COLLIDE_SLOPEH_UR_B:
          list.emplace_back(Geometry{
              min + (min_c == Tileset::COLLIDE_SLOPEH_UR_B ? Tileset::l :
                                                             Tileset::dl),
              max + (max_c == Tileset::COLLIDE_SLOPEH_UR_A ? Tileset::r :
                                                             Tileset::ur)});
          break;
        case Tileset::COLLIDE_SLOPEH_DL_A:
        case Tileset::COLLIDE_SLOPEH_DL_B:
          list.emplace_back(Geometry{
              max + (max_c == Tileset::COLLIDE_SLOPEH_DL_B ? Tileset::r :
                                                             Tileset::ur),
              min + (min_c == Tileset::COLLIDE_SLOPEH_DL_A ? Tileset::l :
                                                             Tileset::dl)});
          break;
        case Tileset::COLLIDE_SLOPEH_DR_A:
        case Tileset::COLLIDE_SLOPEH_DR_B:
          list.emplace_back(Geometry{
              max + (max_c == Tileset::COLLIDE_SLOPEH_DR_A ? Tileset::r :
                                                             Tileset::dr),
              min + (min_c == Tileset::COLLIDE_SLOPEH_DR_B ? Tileset::l :
                                                             Tileset::ul)});
          break;
        case Tileset::COLLIDE_SLOPE2_UL_A:
        case Tileset::COLLIDE_SLOPE2_UL_B:
          list.emplace_back(Geometry{
              min + (min_c == Tileset::COLLIDE_SLOPE2_UL_B ? Tileset::u :
                                                             Tileset::ul),
              max + (max_c == Tileset::COLLIDE_SLOPE2_UL_A ? Tileset::d :
                                                             Tileset::dr)});
          break;
        case Tileset::COLLIDE_SLOPE2_UR_A:
        case Tileset::COLLIDE_SLOPE2_UR_B:
          list.emplace_back(Geometry{
              min + (min_c == Tileset::COLLIDE_SLOPE2_UR_A ? Tileset::d :
                                                             Tileset::dl),
              max + (max_c == Tileset::COLLIDE_SLOPE2_UR_B ? Tileset::u :
                                                             Tileset::ur)});
          break;
        case Tileset::COLLIDE_SLOPE2_DL_A:
        case Tileset::COLLIDE_SLOPE2_DL_B:
          list.emplace_back(Geometry{
              min + (min_c == Tileset::COLLIDE_SLOPE2_DL_A ? Tileset::u :
                                                             Tileset::ur),
              max + (max_c == Tileset::COLLIDE_SLOPE2_DL_B ? Tileset::d :
                                                             Tileset::dl)});
          break;
        case Tileset::COLLIDE_SLOPE2_DR_A:
        case Tileset::COLLIDE_SLOPE2_DR_B:
          list.emplace_back(Geometry{
              max + (max_c == Tileset::COLLIDE_SLOPE2_DR_B ? Tileset::d :
                                                             Tileset::dr),
              min + (min_c == Tileset::COLLIDE_SLOPE2_DR_A ? Tileset::u :
                                                             Tileset::ul)});
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
  // The same technique could be used for each possible sloped gradient, but
  // since these are probably less common and have the nice property of at most
  // one sloped edge per tile, they are handled separately afterwards.
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
    for (y::int32 t = 0; t <= Cell::cell_width; ++t) {
      bool above = local::is_tile_blocked(cell, {t, row - 1}, EDGE_DOWN);
      bool below = local::is_tile_blocked(cell, {t, row}, EDGE_UP);

      Boundary new_boundary = above && !below ? LEFT :
                              below && !above ? RIGHT : NONE;
      if (boundary == new_boundary) {
        continue;
      }

      if (boundary != NONE) {
        y::ivec2 start = Tileset::tile_size * y::ivec2{boundary_start, row};
        y::ivec2 end = Tileset::tile_size * y::ivec2{t, row};

        // Make sure the geometry is always on the right of its boundary.
        list.emplace_back(boundary == RIGHT ?
                          Geometry{start, end} : Geometry{end, start});
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
    for (y::int32 t = 0; t <= Cell::cell_height; ++t) {
      bool left = local::is_tile_blocked(cell, {col - 1, t}, EDGE_RIGHT);
      bool right = local::is_tile_blocked(cell, {col, t}, EDGE_LEFT);

      // Since we're moving downwards, left tile blocked means
      // boundary is actually on the right.
      Boundary new_boundary = left && !right ? RIGHT :
                              right && !left ? LEFT : NONE;
      if (boundary == new_boundary) {
        continue;
      }

      if (boundary != NONE) {
        y::ivec2 start = Tileset::tile_size * y::ivec2{col, boundary_start};
        y::ivec2 end = Tileset::tile_size * y::ivec2{col, t};

        list.emplace_back(boundary == RIGHT ?
                          Geometry{start, end} : Geometry{end, start});
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
    if (c == Tileset::COLLIDE_NONE || c == Tileset::COLLIDE_FULL) {
      continue;
    }
    set.insert(*it);

    // At the same time, add the straight half-length edges. These are not
    // currently handled by the main horizontal/vertical loop.
    // TODO: would be cleaner if we could join them.
    local::add_straight_half_edges(bucket.middle, cell, *it, c);
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
    static void insert(OrderedGeometry& target,
                       const geometry_list& list, const y::ivec2& offset)
    {
      for (const Geometry& g : list) {
        target.insert({g.start + offset, g.end + offset});
      }
    }

    static void merge_loop(
        OrderedGeometry& target,
        const y::ivec2& a_offset, const y::ivec2& b_offset,
        y::int32 a_min, y::int32 a_max, y::int32 b_min, y::int32 b_max,
        y::size& a_index, y::size& b_index, geometry_list& a, geometry_list& b)
    {
      if (a_max < b_min) {
        target.insert({a_offset + a[a_index].start, a_offset + a[a_index].end});
        ++a_index;
        return;
      }
      if (b_max < a_min) {
        target.insert({b_offset + b[b_index].start, b_offset + b[b_index].end});
        ++b_index;
        return;
      }

      // This rest magically works for all cases because the one of the
      // lists has segments stored in reverse.
      if (a_min != b_min) {
        target.insert({a_offset + a[a_index].start, b_offset + b[b_index].end});
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

  _ordered_geometry.clear();
  for (auto it = y::cartesian(min, max); it; ++it) {
    auto jt = _buckets.find(*it);
    if (jt == _buckets.end()) {
      continue;
    }

    // Add all non-edge geometry.
    const bucket& bucket = jt->second;
    const y::ivec2 offset = *it * Tileset::tile_size * Cell::cell_size;
    local::insert(_ordered_geometry, bucket.middle, offset);

    // Where there's no adjacent cell, add the edge geometry.
    if (_buckets.find(*it - y::ivec2{0, 1}) == _buckets.end()) {
      local::insert(_ordered_geometry, bucket.top, offset);
    }
    if (_buckets.find(*it - y::ivec2{1, 0}) == _buckets.end()) {
      local::insert(_ordered_geometry, bucket.left, offset);
    }
    if (_buckets.find(*it + y::ivec2{0, 1}) == _buckets.end()) {
      local::insert(_ordered_geometry, bucket.bottom, offset);
    }
    if (_buckets.find(*it + y::ivec2{1, 0}) == _buckets.end()) {
      local::insert(_ordered_geometry, bucket.right, offset);
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

        local::merge_loop(_ordered_geometry, offset, bottom_offset,
                          top_min, top_max, bottom_min, bottom_max,
                          top_index, bottom_index, top, bottom);
      }
      for (; top_index < top.size(); ++top_index) {
        _ordered_geometry.insert({offset + top[top_index].start,
                                  offset + top[top_index].end});
      }
      for (; bottom_index < bottom.size(); ++bottom_index) {
        _ordered_geometry.insert({bottom_offset + bottom[bottom_index].start,
                                  bottom_offset + bottom[bottom_index].end});
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
        local::merge_loop(_ordered_geometry, right_offset, offset,
                          right_min, right_max, left_min, left_max,
                          right_index, left_index, right, left);
      }
      for (; left_index < left.size(); ++left_index) {
        _ordered_geometry.insert({offset + left[left_index].start,
                                  offset + left[left_index].end});
      }
      for (; right_index < right.size(); ++right_index) {
        _ordered_geometry.insert({right_offset + right[right_index].start,
                                  right_offset + right[right_index].end});
      }
    }
  }
}

WorldWindow::WorldWindow(const CellMap& active_map,
                         const y::ivec2& active_coord)
  : _active_map(&active_map)
  , _active_map_offset(-active_coord)
  , _active_window(new active_window_entry[
      (1 + 2 * active_window_half_size) * (1 + 2 * active_window_half_size)])
{
  update_active_window();
}

void WorldWindow::set_active_map(const CellMap& active_map,
                                 const y::ivec2& active_coord)
{
  if (_active_map == &active_map) {
    set_active_coord(active_coord);
    return;
  }
  _active_map = &active_map;
  _active_map_offset = -active_coord;
  update_active_window();
}

void WorldWindow::set_active_coord(const y::ivec2& active_coord)
{
  move_active_window(_active_map_offset + active_coord);
}

y::ivec2 WorldWindow::get_active_coord() const
{
  return -_active_map_offset;
}

void WorldWindow::move_active_window(const y::ivec2& offset)
{
  if (offset == y::ivec2()) {
    return;
  }
  y::int32 half_size = active_window_half_size;
  y::ivec2 half_v{half_size, half_size};
  _active_map_offset -= offset;

  // Temporary copy of the active window.
  y::unique<active_window_entry[]> copy(
      new active_window_entry[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // Stores which cells in the copy have been used.
  y::unique<bool[]> used(new bool[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // Stores which cells in the new active window have been filled.
  y::unique<bool[]> done(new bool[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // We swap the geometry by offsetting it.
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

const OrderedGeometry& WorldWindow::get_geometry() const
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
  return {blueprint.path,
          y::wvec2(Cell::cell_size * Tileset::tile_size * _active_map_offset +
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
  return _active_map->get_coord(active_window - _active_map_offset);
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
