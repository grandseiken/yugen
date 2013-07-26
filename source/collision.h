#ifndef COLLISION_H
#define COLLISION_H

#include "common.h"
#include "lua.h"
#include "spatial_hash.h"
#include "vector.h"

class RenderUtil;
class WorldWindow;

// A Body is, thus far, a rectangular area of some size, whose center
// is offset some amount from the origin of the source Script.
struct Body : y::no_copy {
  Body();

  // Get bounds (on x-axis). Potentially this could be made virtual and
  // overridden for other shapes.
  typedef y::pair<y::wvec2, y::wvec2> bounds;
  bounds get_bounds(const y::wvec2& origin, y::world rotation) const;
  bounds get_full_rotation_bounds(
      const y::wvec2& origin, y::world rotation, const y::wvec2& offset) const;

  void get_vertices(y::vector<y::wvec2>& output,
                    const y::wvec2& origin, y::world rotation) const;

  y::wvec2 offset;
  y::wvec2 size;
  y::int32 collide_mask;
};

// Keeps a record of Bodies and handles sweeping and collision.
class Collision : public ScriptMap<Body> {
public:

  Collision(const WorldWindow& world);
  ~Collision() override {}

  void render(RenderUtil& util,
              const y::wvec2& camera_min, const y::wvec2& camera_max) const;

  y::wvec2 collider_move(Script& source, const y::wvec2& move) const;
  y::world collider_rotate(Script& source, y::world rotate,
                           const y::wvec2& origin_offset) const;
  bool body_check(const Script& source, const Body& body,
                  y::int32 collide_mask) const;

  // This must be called whenever a Script's position or rotation changes in
  // order to update the bodies in the spatial hash.
  void update_script(const Script& source) const;
  virtual void on_create(const Script& source, Body* obj) override;
  virtual void on_destroy(const Script& source, Body* obj) override;

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
                                const y::wvec2& move,
                                bool tolerance) const;

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

  Body::bounds get_bounds(const entry_list& bodies, y::int32 collide_mask,
                          const y::wvec2& origin, y::world rotation) const;
  Body::bounds get_full_rotation_bounds(
      const entry_list& bodies, y::int32 collide_mask,
      const y::wvec2& origin, y::world rotation, const y::wvec2& offset) const;

  void get_geometries(y::vector<world_geometry>& output,
                      const y::vector<y::wvec2>& vertices) const;

  const WorldWindow& _world;
  mutable SpatialHash<Body*> _spatial_hash;

};

#endif
