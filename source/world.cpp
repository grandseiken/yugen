#include "world.h"

WorldWindow::WorldWindow(const CellMap& active_map,
                         const y::ivec2& active_coord)
  : _active_map(&active_map)
  , _active_map_offset(-active_coord)
  , _active_window(new ActiveWindowEntry[
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
  y::unique<ActiveWindowEntry[]> copy(
      new ActiveWindowEntry[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // Stores which cells in the copy have been used.
  y::unique<bool[]> used(new bool[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // Stores which cells in the new active window have been filled.
  y::unique<bool[]> done(new bool[(1 + 2 * half_size) * (1 + 2 * half_size)]);

  // Copy the active window into the temporary copy.
  for (auto it = y::cartesian(-half_v, y::ivec2{1, 1} + half_v); it; ++it) {
    y::size internal_index = to_internal_index(*it);
    copy[internal_index].blueprint = _active_window[internal_index].blueprint;
    copy[internal_index].cell.swap(_active_window[internal_index].cell);
    used[internal_index] = done[internal_index] = false;
  }

  // Move the cells that are still in view from the copy to the new window.
  for (auto it = y::cartesian(-half_v, y::ivec2{1, 1} + half_v); it; ++it) {
    y::ivec2 source = *it + offset;
    y::size internal_index = to_internal_index(*it);
    if (source >= -half_v && source <= half_v) {
      y::size copy_index = to_internal_index(source);
      _active_window[internal_index].blueprint = copy[copy_index].blueprint;
      _active_window[internal_index].cell.swap(copy[copy_index].cell);
      used[copy_index] = done[internal_index] = true;
    }
  }

  // Create or swap the remaining unfilled cells in the active window.
  for (auto it = y::cartesian(-half_v, y::ivec2{1, 1} + half_v); it; ++it) {
    y::size internal_index = to_internal_index(*it);
    if (done[internal_index]) {
      continue;
    }
    const CellBlueprint* new_blueprint = active_window_target(*it);
    _active_window[internal_index].blueprint = new_blueprint;

    // If the cell happens to be unchanged, swap it in.
    if (!used[internal_index] &&
        copy[internal_index].blueprint == new_blueprint) {
      _active_window[internal_index].cell.swap(copy[internal_index].cell);
      continue;
    }

    // Otherwise create it.
    _active_window[internal_index].cell =
        y::move_unique(new Cell(*new_blueprint));
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
  const y::ivec2 half_v = {active_window_half_size, active_window_half_size};
  for (auto it = y::cartesian(-half_v, y::ivec2{1, 1} + half_v); it; ++it) {
    update_active_window_cell(*it);
  }
}

void WorldWindow::update_active_window_cell(const y::ivec2& v)
{
  y::size internal_index = to_internal_index(v);

  const CellBlueprint* old_blueprint = _active_window[internal_index].blueprint;
  const CellBlueprint* new_blueprint = active_window_target(v);

  if (old_blueprint != new_blueprint) {
    Cell* new_cell = y::null;
    if (new_blueprint) {
      new_cell = new Cell(*new_blueprint);
    }

    _active_window[internal_index].cell = y::move_unique(new_cell);
    _active_window[internal_index].blueprint = new_blueprint;
  }
}

WorldWindow::ActiveWindowEntry::ActiveWindowEntry()
  : blueprint(y::null)
  , cell(y::null)
{
}

World::World()
  : _test_map()
  , _window(_test_map, y::ivec2())
{
}
