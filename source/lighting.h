#ifndef LIGHTING_H
#define LIGHTING_H

#include "common.h"
#include "lua.h"
#include "vector.h"

struct Geometry;
struct OrderedGeometry;
class RenderUtil;
class WorldWindow;

// Point lights only, so far!
// TODO: cone lights, plane lights.
struct Light {
  Light();

  y::world get_max_range() const;

  y::world intensity;
};

// Keeps a record of Lights and handles fancy lighting algorithms.
class Lighting : public ScriptMap<Light> {
public:

  Lighting(const WorldWindow& world);
  virtual ~Lighting() {}

  void render(RenderUtil& util,
              const y::wvec2& camera_min, const y::wvec2& camera_max) const;

private:

  // Internal lighting functions.
  struct world_geometry {
    world_geometry();
    world_geometry(const Geometry& geometry);
    world_geometry(const y::wvec2& start, const y::wvec2& end);

    bool operator==(const world_geometry& g) const;
    bool operator!=(const world_geometry& g) const;

    y::wvec2 start;
    y::wvec2 end;
  };
  typedef y::vector<world_geometry> geometry_entry;
  typedef y::map<y::wvec2, geometry_entry> geometry_map;

  void get_relevant_geometry(y::vector<y::wvec2>& vertex_output,
                             geometry_entry& geometry_output,
                             geometry_map& map_output,
                             const y::wvec2& origin,
                             y::world max_range,
                             const OrderedGeometry& all_geometry) const;

  void trace_light_geometry(y::vector<y::wvec2>& output,
                            y::world max_range,
                            const y::vector<y::wvec2>& vertex_buffer,
                            const geometry_entry& geometry_buffer,
                            const geometry_map& map) const;

  const WorldWindow& _world;

};

#endif
