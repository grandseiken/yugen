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

private:

  y::world get_move_ratio(const Geometry& geometry,
                          const y::wvec2& vertex, const y::wvec2& move) const;

  const WorldWindow& _world;

  typedef y::unique<Body> body_entry;
  // We hold a weak reference to each Script so that we can destroy the bodies
  // whose source Scripts no longer exist.
  struct entry {
    y::wvec2 get_min() const;
    y::wvec2 get_max() const;

    ConstScriptReference ref;
    y::vector<body_entry> list;
  };
  typedef y::map<Script*, entry> body_map;
  body_map _map;

};

#endif
