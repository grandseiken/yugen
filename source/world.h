#ifndef WORLD_H
#define WORLD_H

#include "common.h"
#include "cell.h"

// Sliding window into a Cell source. The source can be changed to simulate
// non-planar geometry.
// TODO: use a WorldSource interface instead of CellMap directly.
class WorldWindow {
public:

  // Boundary width, in cells, around (0, 0) in the active window.
  static const y::int32 active_window_half_size = 1;

  // Initialise world with the given coord of the active map at (0, 0) in the
  // active window.
  WorldWindow(const CellMap& active_map, const y::ivec2& active_coord);

  // Sets the active map with the given coord at (0, 0) in the active window.
  // Avoids reloading cells whenever possible.
  void set_active_map(const CellMap& active_map, const y::ivec2& active_coord);

  // Sets the given coord to (0, 0) in the active window. Avoids reloading cells
  // whenever possible.
  void set_active_coord(const y::ivec2& active_coord);

  // Moves the active window. Avoids reloading cells whenever possible.
  void move_active_window(const y::ivec2& offset);

  // Get (possibly null) blueprint at v in
  // [-half_width, half_width] * [-half_width, half_width].
  const CellBlueprint* get_active_window_blueprint(const y::ivec2& v) const;

  // Get (possibly null) blueprint at (0, 0).
  const CellBlueprint* get_active_window_blueprint() const;

  // Get (possibly null) cell at v in
  // [-half_width, half_width] * [-half_width, half_width].
  Cell* get_active_window_cell(const y::ivec2& v) const;

  // Get (possibly null) cell at (0, 0).
  Cell* get_active_window_cell() const;

private:

  // Give (x, y) in [-half_width, half_width] * [-half_width, half_width].
  static y::size to_internal_index(const y::ivec2& active_window);

  const CellBlueprint* active_window_target(
      const y::ivec2& active_window) const;

  // Update all cells in the active window.
  void update_active_window();

  // Update the single cell at (x, y) in
  // [-half_width, half_width] * [-half_width, half_width].
  void update_active_window_cell(const y::ivec2& v);

  const CellMap* _active_map;
  y::ivec2 _active_map_offset;

  struct ActiveWindowEntry {
    ActiveWindowEntry();

    const CellBlueprint* blueprint;
    y::unique<Cell> cell;
  };

  typedef y::unique<ActiveWindowEntry[]> active_window;
  active_window _active_window;

};

class World {
public:

  World();

private:

  CellMap _test_map;
  WorldWindow _window;

};

#endif
