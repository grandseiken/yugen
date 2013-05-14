#ifndef COLLISION_H
#define COLLISION_H

#include "common.h"
#include "lua.h"
#include "vector.h"

class RenderUtil;
class Script;
class WorldWindow;

// A Body is, thus far, a rectangular area of some size, whose center
// is offset some amount from the origin of the source Script.
struct Body {
  Body(const Script& source);

  const Script& source;
  y::wvec2 offset;
  y::wvec2 size;
};

class Collision : public y::no_copy {
public:

  Collision(const WorldWindow& world);

  Body* create_body(const Script& source);
  void destroy_body(const Script& source, Body* body);
  void destroy_bodies(const Script& source);
  void clean_up();

  void render(RenderUtil& util,
              const y::wvec2& camera_min, const y::wvec2& camera_max) const;

private:

  const WorldWindow& _world;

  typedef y::unique<Body> body_entry;
  // We hold a weak reference to each Script so that we can destroy the bodies
  // whose source Scripts no longer exist.
  struct entry {
    ConstScriptReference ref;
    y::vector<body_entry> list;
  };
  typedef y::map<const Script*, entry> body_map;
  body_map _map;

};

#endif
