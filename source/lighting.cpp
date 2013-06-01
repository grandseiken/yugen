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

  // Set up map from vertices to geometries they are part of.
  geometry_map map;
  for (const auto& bucket : _world.get_geometry().buckets) {
    for (const auto& g : bucket) {
      map[y::wvec2(g.start)].emplace_back(g);
      map[y::wvec2(g.end)].emplace_back(g);
    }
  }

  // Sorts points radially by angle from the origin point using a Graham
  // scan-like algorithm, starting at angle 0 (rightwards) and increasing
  // by angle.
  struct order {
    bool operator()(const vwv& a, const vwv& b) const
    {
      // Eliminate points in opposite half-planes.
      if (a.vec[yy] >= 0 && b.vec[yy] < 0) {
        return true;
      }
      if (a.vec[yy] < 0 && b.vec[yy] >= 0) {
        return false;
      }
      if (a.vec[yy] == 0 && b.vec[yy] == 0) {
        return a.vec[xx] >= 0 && b.vec[xx] >= 0 ?
            a.vec[xx] < b.vec[xx] : a.vec[xx] > b.vec[xx];
      }

      y::world d = a.vec[yy] * b.vec[xx] - a.vec[xx] * b.vec[yy];
      // If d is zero, points are on same half-line so fall back to distance
      // from the origin.
      return d < 0 ||
        (d == 0 && a.vec.length_squared() < b.vec.length_squared());
    }
  };

  y::vector<vwv> vertex_buffer;
  order o;

  source_list sources;
  get_sources(sources);
  for (const Script* s : sources) {
    if (get_list(*s).empty()) {
      continue;
    }
    const y::wvec2 origin = s->get_origin();

    // Check the maximum range among all the Lights.
    y::world max_range = 0;
    for (const entry& light : get_list(*s)) {
      max_range = y::max(max_range, light->get_max_range());
    }
    // TODO: skip if maximum range doesn't overlap camera.

    // Find all the vertices whose geometries intersect the max range square.
    vertex_buffer.clear();
    get_relevant_vertices(vertex_buffer, origin, max_range, map);

    // Perform angular sort.
    std::sort(vertex_buffer.begin(), vertex_buffer.end(), o);

    // Trace the light geometry.
    y::vector<y::wvec2> trace;
    trace_light_geometry(trace, origin, max_range, map, vertex_buffer);
  }
}

Lighting::world_geometry::world_geometry(const Geometry& geometry)
  : start(y::wvec2(geometry.start))
  , end(y::wvec2(geometry.end))
{
}

void Lighting::get_relevant_vertices(y::vector<vwv>& output,
                                     const y::wvec2& origin,
                                     y::world max_range,
                                     const geometry_map& map) const
{
  // We could find only the vertices whose geometries intersect the circle
  // defined by origin and max_range, but that is way more expensive and
  // squares are easier anyway.
  for (const auto& pair : map) {
    bool intersect = false;
    for (const world_geometry& g : pair.second) {
      const y::wvec2 g_s = g.start - origin;
      const y::wvec2 g_e = g.end - origin;

      y::wvec2 min = y::min(g_s, g_e);
      y::wvec2 max = y::max(g_s, g_e);

      // Check bounds.
      if (max[xx] < -max_range || min[xx] >= max_range ||
          max[yy] < -max_range || min[yy] >= max_range) {
        continue;
      }

      // Check equation.
      if (g_s[xx] - g_e[xx] != 0) {
        y::world m = (g_e[yy] - g_s[yy]) / (g_e[xx] - g_s[xx]);
        y::world y_neg = g_e[yy] + m * (g_e[xx] - max_range);
        y::world y_pos = g_e[yy] + m * (g_e[xx] + max_range);

        if ((max_range < y_neg && max_range < y_pos) ||
            (-max_range >= y_neg && -max_range >= y_pos)) {
          continue;
        }
      }

      intersect = true;
      break;
    }

    if (intersect) {
      output.emplace_back(vwv{&pair.first, pair.first - origin});
    }
  }
}

void Lighting::trace_light_geometry(y::vector<y::wvec2>& output,
                                    const y::wvec2& origin,
                                    y::world max_range,
                                    const geometry_map& map,
                                    const y::vector<vwv>& vertex_buffer) const
{
  struct local {
    // Get geometry we care about that a vertex is part of.
    static bool get_incident_geometry(world_geometry& output,
                                      const y::wvec2& origin,
                                      const geometry_map& map, const vwv& v)
    {
      auto it = map.find(*v.v);
      if (it == map.end()) {
        return false;
      }
      for (const world_geometry& g : it->second) {
        // Skip geometry defined in the wrong direction.
        const y::wvec2 g_s = g.start - origin;
        const y::wvec2 g_e = g.end - origin;
        
        if (g_s[yy] * g_e[xx] - g_s[xx] * g_e[yy] >= 0) {
          continue;
        }

        // Take the first geometry line whose other point falls on the correct
        // (positive) side of the line defined by the origin and the first
        // point. This point is part of exactly two geometry lines so, by
        // skipping geometry in the wrong direction, there will be at
        // most one, which is exactly the geometry line whose start-point we're
        // looking at.
        if (v.vec == g_s) {
          output = g;
          return true;
        }
      }
      return false;
    }
  };

  if (vertex_buffer.empty()) {
    // Special case: use the square.
    output.emplace_back(max_range, max_range);
    output.emplace_back(-max_range, max_range);
    output.emplace_back(-max_range, -max_range);
    output.emplace_back(max_range, -max_range);
    return;
  }

  // TODO: algorithm. use the square.
  y::world dist_sq = 0;
  world_geometry incident_geometry({y::ivec2(), y::ivec2()});

  for (y::size i = 0; i < vertex_buffer.size(); ++i) {
    const auto& v = vertex_buffer[i];
    bool on_same_line_as_prev = false;
    if (i > 0) {
      const auto& prev = vertex_buffer[i - 1];
      on_same_line_as_prev =
          v.vec[xx] * prev.vec[yy] - v.vec[yy] - prev.vec[xx] == 0;
    }

    if (!local::get_incident_geometry(incident_geometry, origin, map, v)) {
    }
    dist_sq = v.vec.length_squared();
    (void)dist_sq;
    (void)on_same_line_as_prev;
  }
  (void)output;
}
