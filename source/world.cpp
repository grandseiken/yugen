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
  _active_map = &active_map;
  _active_map_offset = -active_coord;
  update_active_window();
}

y::size World::to_internal_index(y::int32 active_window_x,
                                 y::int32 active_window_y)
{
  const y::int32 half_size = active_window_half_size;
  y::int32 x = active_window_x + half_size;
  y::int32 y = active_window_y + half_size;
  return y * (1 + 2 * half_size) + x;
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
  CellCoord map_pos = CellCoord(x, y) - _active_map_offset;

  const CellBlueprint* old_blueprint = _active_window[internal_index].blueprint;
  const CellBlueprint* new_blueprint = _active_map->get_coord(map_pos);

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
