#include "lighting.h"
#include "world.h"

#include <algorithm>

Light::Light()
  : intensity(1.)
{
}

y::world Light::get_max_range() const
{
  // TODO.
  return intensity * 100;
}

Lighting::Lighting(const WorldWindow& world)
  : _world(world)
{
}

void Lighting::render(
    RenderUtil& util,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  (void)util;
  (void)camera_min;
  (void)camera_max;

  typedef y::vector<world_geometry> geometry_entry;
  struct vwa {
    const y::wvec2* v;
    y::world angle;
  };

  y::map<y::wvec2, geometry_entry> geometry_map;
  for (const auto& bucket : _world.get_geometry().buckets) {
    for (const auto& g : bucket) {
      geometry_map[y::wvec2(g.start)].emplace_back(g);
      geometry_map[y::wvec2(g.end)].emplace_back(g);
    }
  }

  // Sorts points radially by angle from the origin point.
  struct order {
    const y::wvec2& origin;

    order(const y::wvec2& origin)
      : origin(origin)
    {
    }

    bool operator()(const vwa& a, const vwa& b) const
    {
      // TODO: it may be faster to use 2D cross-product as in the Graham scan,
      // i.e.:
      // (a[xx] - origin[xx]) * (b[yy] - origin[yy]) <
      // (b[xx] - origin[xx]) * (a[yy] - origin[yy])
      // with some way of determining the bounds (this simply finds "left-turn"
      // or "right-turn").
      if (a.angle != b.angle) {
        return a.angle < b.angle;
      }
      // Fall back to signed distance.
      // TODO.
      return a.angle < b.angle;
    }
  };

  y::vector<vwa> vertex_buffer;
  source_list sources;
  get_sources(sources);
  for (const Script* s : sources) {
    if (get_list(*s).empty()) {
      continue;
    }

    const y::wvec2& origin = s->get_origin();
    order o(origin);

    // Check the maximum range among all the Lights.
    y::world max_range = 0;
    for (const entry& light : get_list(*s)) {
      max_range = y::max(max_range, light->get_max_range());
    }
    y::world max_range_sq = max_range * max_range;

    // Find all the vertices close enough to the Script.
    vertex_buffer.clear();
    for (const auto& p : geometry_map) {
      if ((p.first - origin).length_squared() > max_range_sq) {
        continue;
      }

      vertex_buffer.emplace_back(vwa{&p.first, (p.first - origin).angle()});
    }

    // Perform angular sort.
    std::sort(vertex_buffer.begin(), vertex_buffer.end(), o);
  }
}

Lighting::world_geometry::world_geometry(const Geometry& geometry)
  : start(y::wvec2(geometry.start))
  , end(y::wvec2(geometry.end))
{
}
