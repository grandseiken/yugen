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
  _buckets[a] = y::move(_buckets[b]);
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

  // Strategy: right now we only handle collision of full square tiles. Treat
  // horizontal and vertical edges completely separately. Loop over the line
  // segments forming the edges of tiles, adding geometry when the collision
  // of the tiles on either side of the edge differs. When this pattern
  // continues for several tiles, don't add the geometry until we reach the end.

  // When it comes time to add support for irregular collision shapes, it may
  // be faster to keep this set-up, and handle non-full tiles separately after.
  // It would be possible to use the same strategy as above for e.g. slopes,
  // but full-square is the most common case so it's probably a waste to loop
  // over every possible sloped edge as well. The straight edges of irregular
  // tiles can be easily handled by a small modification to the is_tile_blocked
  // function.

  struct local {
    static bool is_tile_blocked(const CellBlueprint& cell, const y::ivec2& v)
    {
      const Tile& tile = cell.get_tile(collision_layer, v);
      return tile.tileset &&
          tile.tileset->get_collision(tile.index) == Tileset::COLLIDE_FULL;
    }
  };
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
      bool above = row == 0 || t == Cell::cell_width ||
          local::is_tile_blocked(cell, {t, row - 1});
      bool below = row == Cell::cell_height || t == Cell::cell_width ||
          local::is_tile_blocked(cell, {t, row});

      Boundary new_boundary = above && !below ? LEFT :
                              below && !above ? RIGHT : NONE;
      if (boundary == new_boundary) {
        continue;
      }

      if (boundary != NONE) {
        y::ivec2 start = Tileset::tile_size * y::ivec2{boundary_start, row};
        y::ivec2 end = Tileset::tile_size * y::ivec2{t, row};

        // Make sure the geometry is always on the right of its boundary.
        list.push_back(boundary == RIGHT ?
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
      bool left = col == 0 || t == Cell::cell_height ||
          local::is_tile_blocked(cell, {col - 1, t});
      bool right = col == Cell::cell_width || t == Cell::cell_height ||
          local::is_tile_blocked(cell, {col, t});

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

        list.push_back(boundary == RIGHT ?
                       Geometry{start, end} : Geometry{end, start});
      }
      if (new_boundary != NONE) {
        boundary_start = t;
      }
      boundary = new_boundary;
    }
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
    _refreshed_cells.push_back(*it);
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
    _refreshed_cells.push_back(v);
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
