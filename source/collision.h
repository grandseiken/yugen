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

  const Script& source;
  y::wvec2 offset;
  y::wvec2 size;
  y::int32 collide_mask;
};

// Keeps a record of Bodies and handles sweeping and collision.
class Collision : public y::no_copy {
public:

  Collision(const WorldWindow& world);

  Body* create_body(Script& source);
  void destroy_body(const Script& source, Body* body);
  void destroy_bodies(const Script& source);
  void clean_up();

  void render(RenderUtil& util,
              const y::wvec2& camera_min, const y::wvec2& camera_max) const;

  void collider_move(Script& source, const y::wvec2& move) const;
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

  bool has_intersection(const y::vector<world_geometry>& a,
                        const world_geometry& b) const;
  bool has_intersection(const world_geometry& a,
                        const world_geometry& b) const;

  const WorldWindow& _world;

  typedef y::unique<Body> body_entry;
  // We hold a weak reference to each Script so that we can destroy the bodies
  // whose source Scripts no longer exist.
  struct entry {
    y::wvec2 get_min(y::int32 collide_mask) const;
    y::wvec2 get_max(y::int32 collide_mask) const;

    ConstScriptReference ref;
    y::vector<body_entry> list;
  };
  typedef y::map<Script*, entry> body_map;
  body_map _map;

};

#endif
