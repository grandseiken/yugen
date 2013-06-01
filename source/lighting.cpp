#include "lighting.h"
#include "world.h"

#include <algorithm>
#include <boost/functional/hash.hpp>

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

  // Set up list of all geometry.
  geometry_entry all_geometry;
  for (const auto& bucket : _world.get_geometry().buckets) {
    for (const auto& g : bucket) {
      all_geometry.emplace_back(g);
    }
  }

  // Sorts points radially by angle from the origin point using a Graham
  // scan-like algorithm, starting at angle 0 (rightwards) and increasing
  // by angle.
  struct order {
    bool operator()(const y::wvec2& a, const y::wvec2& b) const
    {
      // Eliminate points in opposite half-planes.
      if (a[yy] >= 0 && b[yy] < 0) {
        return true;
      }
      if (a[yy] < 0 && b[yy] >= 0) {
        return false;
      }
      if (a[yy] == 0 && b[yy] == 0) {
        return a[xx] >= 0 && b[xx] >= 0 ? a[xx] < b[xx] : a[xx] > b[xx];
      }

      y::world d = a[yy] * b[xx] - a[xx] * b[yy];
      // If d is zero, points are on same half-line so fall back to distance
      // from the origin.
      return d < 0 ||
        (d == 0 && a.length_squared() < b.length_squared());
    }
  };

  y::vector<y::wvec2> vertex_buffer;
  geometry_entry geometry_buffer;
  geometry_map map;
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
    // TODO: cache results if they don't change per-frame. Split this into
    // a tracing phase and a drawing phase.

    // Find all the geometries that intersect the max-range square and their
    // vertices, translated respective to origin.
    vertex_buffer.clear();
    geometry_buffer.clear();
    map.clear();
    get_relevant_geometry(vertex_buffer, geometry_buffer, map,
                          origin, max_range, all_geometry);

    // Perform angular sort.
    std::sort(vertex_buffer.begin(), vertex_buffer.end(), o);

    // Trace the light geometry.
    y::vector<y::wvec2> trace;
    trace_light_geometry(trace, max_range,
                         vertex_buffer, geometry_buffer, map);
  }
}

Lighting::world_geometry::world_geometry(const Geometry& geometry)
  : start(y::wvec2(geometry.start))
  , end(y::wvec2(geometry.end))
{
}

Lighting::world_geometry::world_geometry(const y::wvec2& start,
                                         const y::wvec2& end)
  : start(start)
  , end(end)
{
}

bool Lighting::world_geometry::operator==(const world_geometry& g) const
{
  return start == g.start && end == g.end;
}

bool Lighting::world_geometry::operator!=(const world_geometry& g) const
{
  return !operator==(g);
}

void Lighting::get_relevant_geometry(y::vector<y::wvec2>& vertex_output,
                                     geometry_entry& geometry_output,
                                     geometry_map& map_output,
                                     const y::wvec2& origin,
                                     y::world max_range,
                                     const geometry_entry& all_geometry) const
{
  // We could find only the vertices whose geometries intersect the circle
  // defined by origin and max_range, but that is way more expensive and
  // squares are easier anyway.
  y::set<y::wvec2> added_vertices;
  for (const world_geometry& g : all_geometry) {
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

    // Exclude geometries which are defined in the wrong direction.
    if (g_s[yy] * g_e[xx] - g_s[xx] * g_e[yy] >= 0) {
      continue;
    }

    geometry_output.emplace_back(g_s, g_e);
    map_output[g_s].emplace_back(g_s, g_e);
    map_output[g_e].emplace_back(g_s, g_e);
  }
  for (const auto& pair : map_output) {
    vertex_output.emplace_back(pair.first);
  }
}

void Lighting::trace_light_geometry(y::vector<y::wvec2>& output,
                                    y::world max_range,
                                    const y::vector<y::wvec2>& vertex_buffer,
                                    const geometry_entry& geometry_buffer,
                                    const geometry_map& map) const
{
  struct wg_hash {
    y::size operator()(const world_geometry& g) const
    {
      y::size seed = 0;
      boost::hash_combine(seed, g.start[xx]);
      boost::hash_combine(seed, g.start[yy]);
      boost::hash_combine(seed, g.end[xx]);
      boost::hash_combine(seed, g.end[yy]);
      return seed;
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

  typedef y::set<world_geometry, wg_hash> geometry_set;
  geometry_set stack;

  // Initialise the stack with geometry that intersects the line from the origin
  // to the first vertex (but doesn't start exactly on it). To make sure we get
  // only geometry crossing the positive half of the line and not the negative
  // half, make sure line is defined in correct direction.
  const y::wvec2& first_vec = vertex_buffer[0];
  for (const world_geometry& g : geometry_buffer) {
    y::world d_s = first_vec[xx] * g.start[yy] - first_vec[yy] * g.start[xx];
    y::world d_e = first_vec[xx] * g.end[yy] - first_vec[yy] * g.end[xx];
    // If d_e < 0 && d_s > 0 then line crosses the negative half.
    if (d_s < 0 && d_e >= 0) {
      stack.insert(g);
    }
  }  

  // Algorithm: loop through the vertices. When it's the end of a geometry line
  // in the stack, relative to the angular sweep direction, remove from the
  // stack. When it's the start, add to the stack. Since we've excluded geometry
  // defined opposite the sweep direction, this corresponds exactly to whether
  // the vertice is the start or end point of the geometry.
  y::world dist_sq = 0;

  for (y::size i = 0; i < vertex_buffer.size(); ++i) {
    const auto& v = vertex_buffer[i];
    bool on_same_line_as_prev = false;
    if (i > 0) {
      const auto& prev = vertex_buffer[i - 1];
      on_same_line_as_prev =
          v[xx] * prev[yy] - v[yy] - prev[xx] == 0;
    }
    dist_sq = v.length_squared();

    auto it = map.find(v);
    if (it == map.end()) {
      continue;
    }
    // Add or remove from stack as appropriate.
    for (const world_geometry& g : it->second) {
      if (v == g.start) {
        stack.insert(g);
      }
      else {
        stack.erase(g);
      }
    }

    // Are these even useful?
    (void)dist_sq;
    (void)on_same_line_as_prev;
  }
}
