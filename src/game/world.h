#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "../data/cell.h"
#include "../spatial_hash.h"

// A collision boundary line. By convention geometry is stored in clockwise
// order; that is, when facing from start to end the solid geometry lies on the
// right-hand side.
struct Geometry {
  Geometry(const y::ivec2& start, const y::ivec2& end, bool external = false);

  y::ivec2 start;
  y::ivec2 end;

  bool operator==(const Geometry& g) const;
  bool operator!=(const Geometry& g) const;

  // True if the geometry is part of the external wall rather than actual tiles.
  bool external;
};

namespace std {
  template<>
  struct hash<Geometry> {
    y::size operator()(const Geometry& g) const;
  };
}

// Stores world geometry.
class WorldGeometry : public y::no_copy {
public:

  WorldGeometry();
  ~WorldGeometry();

  typedef SpatialHash<Geometry, y::world, 2> geometry_hash;

  // Set the geometry at a coordinate by calculating it from a CellBlueprint.
  void merge_geometry(const CellBlueprint& cell, const y::ivec2& coord);
  // Clear the ceometry at a coordinate.
  void clear_geometry(const y::ivec2& coord);
  // Swap the geometry between two coordinates.
  void swap_geometry(const y::ivec2& a, const y::ivec2& b);

  // Get current geometry for the whole world.
  const geometry_hash& get_geometry() const;

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
  mutable geometry_hash _geometry_hash;
  mutable bool _dirty;

  void calculate_geometry(bucket& bucket, const CellBlueprint& cell);
  void merge_all_geometry() const;

};

struct WorldScript {
  y::string path;
  y::wvec2 origin;
  y::wvec2 region;
};

// Interface for supplying cells to the WorldWindow. WorldSources must be
// immutable and must also never be destroyed while Scripts still exist
// in the game world that they have been sourced from.
class WorldSource : public y::no_copy {
public:

  // Pass a unique ID based on the type so we can distinguish equality.
  WorldSource(y::size type_id);
  virtual ~WorldSource() {}

  // Subclasses must override to call types_equal(source) and check data.
  virtual bool operator==(const WorldSource& source) const = 0;
  bool operator!=(const WorldSource& source) const;

  // Subclasses must override to hash get_type_id() and the data.
  virtual void hash_combine(y::size& seed) const = 0;

  // Get the cell at a particular coordinate.
  virtual const CellBlueprint* get_coord(const y::ivec2& coord) const = 0;
  // Get all the scripts.
  virtual const CellMap::script_list& get_scripts() const = 0;

protected:

  bool types_equal(const WorldSource& source) const;
  y::size get_type_id() const;

private:

  y::size _type_id;

};

// Default WorldSource based on a CellMap.
class CellMapSource : public WorldSource {
public:

  const y::size type_id = 0xce11;

  CellMapSource(const CellMap& map);
  ~CellMapSource() override {}

  bool operator==(const WorldSource& source) const override;
  void hash_combine(y::size& seed) const override;

  const CellBlueprint* get_coord(const y::ivec2& coord) const override;
  virtual const CellMap::script_list& get_scripts() const override;

private:

  const CellMap& _map;

};

// Sliding window into a Cell source. The source can be changed to simulate
// non-planar geometry.
class WorldWindow : public y::no_copy {
public:

  // Boundary width, in cells, around (0, 0) in the active window.
  static const y::int32 active_window_half_size = 1;

  // Initialise world with the given coord of the active map at (0, 0) in the
  // active window. Source is owned by caller and must be preserved.
  WorldWindow(const WorldSource& active_source, const y::ivec2& active_coord);

  // Sets the active map with the given coord at (0, 0) in the active window.
  // Avoids reloading cells whenever possible.
  void set_active_source(const WorldSource& active_source,
                         const y::ivec2& active_coord);

  // Sets the given coord to (0, 0) in the active window. Avoids reloading cells
  // whenever possible.
  void set_active_coord(const y::ivec2& active_coord);

  // Get active window center coord.
  y::ivec2 get_active_coord() const;

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
  const WorldGeometry::geometry_hash& get_geometry() const;

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

  const WorldSource* _active_source;
  y::ivec2 _active_source_offset;

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
