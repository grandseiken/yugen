#include "world.h"

World::World(const CellMap& active_map, const CellCoord& active_coord)
  : _active_map(&active_map)
  , _active_map_offset(-active_coord)
  , _active_window(new ActiveWindowEntry[
      (1 + 2 * active_window_half_size) * (1 + 2 * active_window_half_size)])
{
  update_active_window();
}

void World::set_active_map(const CellMap& active_map,
                           const CellCoord& active_coord)
{
  if (_active_map == &active_map) {
    set_active_coord(active_coord);
    return;
  }
  _active_map = &active_map;
  _active_map_offset = -active_coord;
  update_active_window();
}

void World::set_active_coord(const CellCoord& active_coord)
{
  move_active_window(_active_map_offset + active_coord);
}

void World::move_active_window(const CellCoord& offset)
{
  if (offset == CellCoord(0, 0)) {
    return;
  }
  const y::int32 half_size = active_window_half_size;
  _active_map_offset -= offset;

  // Temporary copy of the active window.
  y::unique<ActiveWindowEntry[]> copy(
      new ActiveWindowEntry[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // Stores which cells in the copy have been used.
  y::unique<bool[]> used(new bool[(1 + 2 * half_size) * (1 + 2 * half_size)]);
  // Stores which cells in the new active window have been filled.
  y::unique<bool[]> done(new bool[(1 + 2 * half_size) * (1 + 2 * half_size)]);

  // Copy the active window into the temporary copy.
  for (y::int32 x = -half_size; x <= half_size; ++x) {
    for (y::int32 y = -half_size; y <= half_size; ++y) {
      y::size internal_index = to_internal_index(x, y);
      copy[internal_index].blueprint = _active_window[internal_index].blueprint;
      copy[internal_index].cell.swap(_active_window[internal_index].cell);
      used[internal_index] = done[internal_index] = false;
    }
  }

  // Move the cells that are still in view from the copy to the new window.
  for (y::int32 x = -half_size; x <= half_size; ++x) {
    for (y::int32 y = -half_size; y <= half_size; ++y) {
      CellCoord source = CellCoord(x, y) + offset;
      y::size internal_index = to_internal_index(x, y);
      if (source.x >= -half_size && source.x <= half_size &&
          source.y >= -half_size && source.y <= half_size) {
        y::size copy_index = to_internal_index(source.x, source.y);
        _active_window[internal_index].blueprint = copy[copy_index].blueprint;
        _active_window[internal_index].cell.swap(copy[copy_index].cell);
        used[copy_index] = done[internal_index] = true;
      }
    }
  }

  // Create or swap the remaining unfilled cells in the active window.
  for (y::int32 x = -half_size; x <= half_size; ++x) {
    for (y::int32 y = -half_size; y <= half_size; ++y) {
      y::size internal_index = to_internal_index(x, y);
      if (done[internal_index]) {
        continue;
      }
      const CellBlueprint* new_blueprint = active_window_target(x, y);
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
}

y::size World::to_internal_index(y::int32 active_window_x,
                                 y::int32 active_window_y)
{
  const y::int32 half_size = active_window_half_size;
  y::int32 x = active_window_x + half_size;
  y::int32 y = active_window_y + half_size;
  return y * (1 + 2 * half_size) + x;
}

const CellBlueprint* World::active_window_target(y::int32 active_window_x,
                                                 y::int32 active_window_y) const
{
  CellCoord map_pos =
      CellCoord(active_window_x, active_window_y) - _active_map_offset;
  return _active_map->get_coord(map_pos);
}

void World::update_active_window()
{
  const y::int32 half_size = active_window_half_size;
  for (y::int32 x = -half_size; x <= half_size; ++x) {
    for (y::int32 y = -half_size; y <= half_size; ++y) {
      update_active_window_cell(x, y);
    }
  }
}

void World::update_active_window_cell(y::int32 x, y::int32 y)
{
  y::size internal_index = to_internal_index(x, y);

  const CellBlueprint* old_blueprint = _active_window[internal_index].blueprint;
  const CellBlueprint* new_blueprint = active_window_target(x, y);

  if (old_blueprint != new_blueprint) {
    Cell* new_cell = y::null;
    if (new_blueprint) {
      new_cell = new Cell(*new_blueprint);
    }

    _active_window[internal_index].cell = y::move_unique(new_cell);
    _active_window[internal_index].blueprint = new_blueprint;
  }
}

World::ActiveWindowEntry::ActiveWindowEntry()
  : blueprint(y::null)
  , cell(y::null)
{
}
