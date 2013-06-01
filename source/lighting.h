#ifndef LIGHTING_H
#define LIGHTING_H

#include "common.h"
#include "lua.h"
#include "vector.h"

struct Geometry;
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
    world_geometry(const Geometry& geometry);

    y::wvec2 start;
    y::wvec2 end;
  };
  struct vwv {
    const y::wvec2* v;
    y::wvec2 vec;
  };
  typedef y::vector<world_geometry> geometry_entry;
  typedef y::map<y::wvec2, geometry_entry> geometry_map;

  void get_relevant_vertices(y::vector<vwv>& output,
                             const y::wvec2& origin,
                             y::world max_range,
                             const geometry_map& map) const;

  void trace_light_geometry(y::vector<y::wvec2>& output,
                            const y::wvec2& origin,
                            y::world max_range,
                            const geometry_map& map,
                            const y::vector<vwv>& vertex_buffer) const;

  const WorldWindow& _world;

};

#endif
