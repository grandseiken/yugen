#ifndef WORLD_H
#define WORLD_H

#include "common.h"
#include "cell.h"

// A collision boundary line. By convention geometry is stored in clockwise
// order; that is, when facing from start to end the solid geometry lies on the
// right-hand side.
struct Geometry {
  y::ivec2 start;
  y::ivec2 end;

  struct Order {
    bool operator()(const Geometry&, const Geometry& b) const;
  };
};

typedef y::ordered_set<Geometry, Geometry::Order> OrderedGeometry;

// Stores world geometry.
class WorldGeometry {
public:

  WorldGeometry();

  void merge_geometry(const CellBlueprint& cell, const y::ivec2& coord);
  void clear_geometry(const y::ivec2& coord);
  void swap_geometry(const y::ivec2& a, const y::ivec2& b);
  bool has_geometry(const y::ivec2& coord) const;

  const OrderedGeometry& get_geometry() const;

private:

  typedef y::vector<Geometry> GeometryList;

  struct Bucket {
    GeometryList middle;
    GeometryList top;
    GeometryList bottom;
    GeometryList left;
    GeometryList right;
  };

  y::map<y::ivec2, Bucket> _buckets;
  mutable OrderedGeometry _ordered_geometry;
  mutable bool _dirty;

  void calculate_geometry(Bucket& bucket, const CellBlueprint& cell);
  void merge_all_geometry() const;

};

// Sliding window into a Cell source. The source can be changed to simulate
// non-planar geometry.
// TODO: use a WorldSource interface instead of CellMap directly?
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

  // Iterate through window coordinates.
  y::ivec2_iterator get_cartesian() const;

  // Get geometry.
  const OrderedGeometry& get_geometry() const;

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
  WorldGeometry _active_geometry;

};

#endif
