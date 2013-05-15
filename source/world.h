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

  struct order {
    bool operator()(const Geometry&, const Geometry& b) const;
  };
};

// An ordered bucket of geometry orders its members by their minimum
// x-coordinate. The OrderedGeometry groups several buckets together, one per
// x-cell, each bucket's elements having maximum x-coordinate in that cell.
typedef y::ordered_set<Geometry, Geometry::order> OrderedBucket;
struct OrderedGeometry {
  OrderedGeometry();

  void insert(const Geometry& g);
  void clear();
  y::int32 get_max_for_bucket(y::size index) const;

  y::vector<OrderedBucket> buckets;
};

// Stores world geometry.
class WorldGeometry {
public:

  WorldGeometry();

  // Set the geometry at a coordinate by calculating it from a CellBlueprint.
  void merge_geometry(const CellBlueprint& cell, const y::ivec2& coord);
  // Clear the ceometry at a coordinate.
  void clear_geometry(const y::ivec2& coord);
  // Swap the geometry between two coordinates.
  void swap_geometry(const y::ivec2& a, const y::ivec2& b);

  // Get current geometry for the whole world.
  const OrderedGeometry& get_geometry() const;

private:

  typedef y::vector<Geometry> geometry_list;

  struct bucket {
    geometry_list middle;
    geometry_list top;
    geometry_list bottom;
    geometry_list left;
    geometry_list right;
  };

  y::map<y::ivec2, bucket> _buckets;
  mutable OrderedGeometry _ordered_geometry;
  mutable bool _dirty;

  void calculate_geometry(bucket& bucket, const CellBlueprint& cell);
  void merge_all_geometry() const;

};

struct WorldScript {
  y::string path;
  y::wvec2 origin;
  y::wvec2 region;
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

  // After window operations, there may be new Scripts that should be
  // instantiated. These functions report which cells should have their scripts
  // refreshed.
  typedef y::vector<y::ivec2> cell_list;
  const cell_list& get_refreshed_cells() const;
  void clear_refreshed_cells();

  // Translate script to relative coordinates.
  WorldScript script_blueprint_to_world_script(
      const ScriptBlueprint& blueprint) const;

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

  struct active_window_entry {
    active_window_entry();

    const CellBlueprint* blueprint;
    y::unique<Cell> cell;
  };

  typedef y::unique<active_window_entry[]> active_window;
  active_window _active_window;
  WorldGeometry _active_geometry;

  cell_list _refreshed_cells;

};

#endif
