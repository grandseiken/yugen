#ifndef COLLISION_H
#define COLLISION_H

#include "common.h"
#include "lua.h"
#include "vector.h"

class RenderUtil;
class Script;
class WorldWindow;
struct Geometry;

// A Body is, thus far, a rectangular area of some size, whose center
// is offset some amount from the origin of the source Script.
struct Body {
  Body(const Script& source);

  // Get bounds (on x-axis). Potentially this could be made virtual and
  // overridden for other shapes.
  y::wvec2 get_min() const;
  y::wvec2 get_max() const;

  void get_vertices(y::vector<y::wvec2>& output,
                    const y::wvec2& origin, y::world rotation) const;

  const Script& source;
  y::wvec2 offset;
  y::wvec2 size;
  y::int32 collide_mask;
};

// Keeps a record of Bodies and handles sweeping and collision.
class Collision : public ScriptMap<Body> {
public:

  Collision(const WorldWindow& world);

  void render(RenderUtil& util,
              const y::wvec2& camera_min, const y::wvec2& camera_max) const;

  void collider_move(Script& source, const y::wvec2& move) const;
  // TODO: allow rotation about arbitrary points rather than just the origin.
  void collider_rotate(Script& source, y::world rotate) const;
  bool body_check(const Script& source, const Body& body,
                  y::int32 collide_mask) const;

private:

  // Internal collision functions.
  struct world_geometry {
    y::wvec2 start;
    y::wvec2 end;
  };

  y::world get_projection_ratio(const y::vector<world_geometry>& geometry,
                                const y::vector<y::wvec2>& vertices,
                                const y::wvec2& move) const;
  y::world get_projection_ratio(const world_geometry& geometry,
                                const y::vector<y::wvec2>& vertices,
                                const y::wvec2& move) const;
  y::world get_projection_ratio(const world_geometry& geometry,
                                const y::wvec2& vertex,
                                const y::wvec2& move) const;

  y::world get_arc_projection(const y::vector<world_geometry>& geometry,
                              const y::vector<y::wvec2>& vertices,
                              const y::wvec2& origin, y::world rotation) const;
  y::world get_arc_projection(const world_geometry& geometry,
                              const y::vector<y::wvec2>& vertices,
                              const y::wvec2& origin, y::world rotation) const;
  y::world get_arc_projection(const world_geometry& geometry,
                              const y::wvec2& vertex,
                              const y::wvec2& origin, y::world rotation) const;

  bool has_intersection(const y::vector<world_geometry>& a,
                        const world_geometry& b) const;
  bool has_intersection(const world_geometry& a,
                        const world_geometry& b) const;

  y::wvec2 get_min(const entry_list& bodies, y::int32 collide_mask) const;
  y::wvec2 get_max(const entry_list& bodies, y::int32 collide_mask) const;
  void get_geometries(y::vector<world_geometry>& output,
                      const y::vector<y::wvec2>& vertices) const;

  const WorldWindow& _world;

};

#endif
