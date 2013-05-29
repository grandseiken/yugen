#ifndef LIGHTING_H
#define LIGHTING_H

#include "common.h"
#include "lua.h"
#include "vector.h"

class RenderUtil;
class WorldWindow;

// Point lights only, so far!
// TODO: cone lights, plane lights.
struct Light {
  Light();

  y::world intensity;
};

// Keeps a record of Lights and handles sweeping and collision.
class Lighting : public ScriptMap<Light> {
public:

  Lighting(const WorldWindow& world);
  virtual ~Lighting() {}

  void render(RenderUtil& util,
              const y::wvec2& camera_min, const y::wvec2& camera_max) const;

private:

  // Internal lighting functions.
  struct world_geometry {
    y::wvec2 start;
    y::wvec2 end;
  };

  const WorldWindow& _world;

};

#endif
