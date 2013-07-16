#include "lighting.h"
#include "gl_util.h"
#include "render_util.h"
#include "world.h"

#include <algorithm>
#include <boost/functional/hash.hpp>

Light::Light()
  : full_range(1.)
  , falloff_range(1.)
  , layer_value(0.)
  , colour{1.f, 1.f, 1.f, 1.f}
{
}

y::world Light::get_max_range() const
{
  return full_range + falloff_range;
}

Lighting::Lighting(const WorldWindow& world, GlUtil& gl)
  : _world(world)
  , _gl(gl)
  , _point_light_program(gl.make_unique_program({
        "/shaders/point_light.v.glsl",
        "/shaders/point_light.f.glsl"}))
  , _point_light_specular_program(gl.make_unique_program({
        "/shaders/point_light_specular.v.glsl",
        "/shaders/point_light_specular.f.glsl"}))
  , _tri_buffer(gl.make_unique_buffer<GLfloat, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _origin_buffer(gl.make_unique_buffer<GLfloat, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _range_buffer(gl.make_unique_buffer<GLfloat, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _colour_buffer(gl.make_unique_buffer<GLfloat, 4>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _layering_buffer(gl.make_unique_buffer<GLfloat, 1>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _element_buffer(gl.make_unique_buffer<GLushort, 1>(
        GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW))
{
}

bool Lighting::trace_key::operator==(const trace_key& key) const
{
  return origin == key.origin && max_range == key.max_range;
}

bool Lighting::trace_key::operator!=(const trace_key& key) const
{
  return !operator==(key);
}

y::size Lighting::trace_key_hash::operator()(const trace_key& key) const
{
  y::size seed = 0;
  boost::hash_combine(seed, key.origin[xx]);
  boost::hash_combine(seed, key.origin[yy]);
  boost::hash_combine(seed, key.max_range);
  return seed;
}

void Lighting::recalculate_traces(
    const y::wvec2& camera_min, const y::wvec2& camera_max)
{
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

  // TODO: we could also limit max_range to camera bounds. However, this blows
  // the cache so it's not necessarily a good idea. In fact, it's certainly
  // only worth it for lights which are moving a lot (and also have a large
  // max-range), and so won't be cached anyway. Perhaps we can set this as a
  // hint somehow.
  y::set<trace_key, trace_key_hash> trace_preserve;

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

    // Skip if maximum range doesn't overlap camera.
    if (!(origin + y::wvec2{max_range, max_range} >= camera_min &&
          origin + y::wvec2{-max_range, -max_range} < camera_max)) {
      continue;
    }

    trace_key key{origin, max_range};
    trace_preserve.insert(key);

    // If the result is cached, we don't need to recalculate it.
    if (_trace_results.find(key) != _trace_results.end()) {
      continue;
    }

    // Find all the geometries that intersect the max-range square and their
    // vertices, translated respective to origin.
    vertex_buffer.clear();
    geometry_buffer.clear();
    map.clear();
    get_relevant_geometry(vertex_buffer, geometry_buffer, map,
                          origin, max_range, _world.get_geometry());

    // Perform angular sort.
    std::sort(vertex_buffer.begin(), vertex_buffer.end(), o);

    // Trace the light geometry.
    trace_light_geometry(_trace_results[key], max_range,
                         vertex_buffer, geometry_buffer, map);
  }

  // Get rid of the cached results we didn't use this frame.
  for (auto it = _trace_results.begin(); it != _trace_results.end();) {
    if (trace_preserve.find(it->first) != trace_preserve.end()) {
      ++it;
    }
    else {
      it = _trace_results.erase(it);
    }
  }
}

void Lighting::clear_results_and_cache()
{
  _trace_results.clear();
}

const Lighting::trace_results& Lighting::get_traces() const
{
  return _trace_results;
}

void Lighting::render_traces(
    RenderUtil& util,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  y::vector<RenderUtil::line> lines;

  for (const auto& pair : get_traces()) {
    for (y::size i = 0; i < pair.second.size(); ++i) {
      y::wvec2 a = pair.first.origin + pair.second[i];
      y::wvec2 b = pair.first.origin + pair.second[(1 + i) %
                                                   pair.second.size()];
      y::wvec2 c = pair.first.origin + pair.second[
          (pair.second.size() + i - 1) % pair.second.size()];

      const y::wvec2 max = y::max(a, b);
      const y::wvec2 min = y::min(a, b);
      if (max >= camera_min && min < camera_max) {
        lines.emplace_back(RenderUtil::line{y::fvec2(a), y::fvec2(b)});
        if (a != c) {
          util.render_fill(
              y::fvec2(a) - y::fvec2{2.f, 2.f},
              y::fvec2{4.f, 4.f}, a == b ? y::fvec4{0.f, 0.f, 1.f, .5f} :
                                   i % 2 ? y::fvec4{1.f, 0.f, 0.f, .5f} :
                                           y::fvec4{0.f, 1.f, 0.f, .5f});
        }
      }
    }
  }
  util.render_lines(lines, {1.f, 1.f, 1.f, .5f});
}

void Lighting::render_lightbuffer(
    RenderUtil& util, const GlFramebuffer& normalbuffer,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  render_internal(util, normalbuffer, camera_min, camera_max, false);
}

void Lighting::render_specularbuffer(
    RenderUtil& util, const GlFramebuffer& normalbuffer,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  render_internal(util, normalbuffer, camera_min, camera_max, true);
}

void Lighting::render_internal(
    RenderUtil& util, const GlFramebuffer& normalbuffer,
    const y::wvec2& camera_min, const y::wvec2& camera_max,
    bool specular) const
{
  if (!(util.get_resolution() >= y::ivec2())) {
    return;
  }
  y::vector<GLfloat> tri_data;
  y::vector<GLfloat> origin_data;
  y::vector<GLfloat> range_data;
  y::vector<GLfloat> colour_data;
  y::vector<GLfloat> layering_data;
  y::vector<GLushort> element_data;

  source_list sources;
  get_sources(sources);
  for (const Script* s : sources) {
    if (get_list(*s).empty()) {
      continue;
    }
    const y::wvec2& origin = s->get_origin();

    y::wvec2 min = camera_min - origin;
    y::wvec2 max = camera_max - origin;

    y::world max_range = 0;
    for (const entry& light : get_list(*s)) {
      max_range = y::max(max_range, light->get_max_range());
    }

    trace_key key{origin, max_range};
    auto it = _trace_results.find(key);
    if (it == _trace_results.end()) {
      continue;
    }
    const light_trace& trace = it->second;
    if (!trace.size()) {
      continue;
    }

    for (const entry& light : get_list(*s)) {
      // Arranging in a triangle fan causes tears in the triangles due to slight
      // inaccuracies, so we use a fan with three triangles per actual triangle
      // to make sure the edges line up exactly.
      // TODO: there is still occassional tearing.
      y::size origin_index = tri_data.size() / 2;

      tri_data.emplace_back(origin[xx]);
      tri_data.emplace_back(origin[yy]);
      origin_data.emplace_back(origin[xx]);
      origin_data.emplace_back(origin[yy]);
      range_data.emplace_back(light->full_range);
      range_data.emplace_back(light->falloff_range);
      layering_data.emplace_back(light->layer_value);
      if (!specular) {
        colour_data.emplace_back(light->colour[rr]);
        colour_data.emplace_back(light->colour[gg]);
        colour_data.emplace_back(light->colour[bb]);
        colour_data.emplace_back(light->colour[aa]);
      }

      // Set up the vertices.
      for (y::size i = 0; i < trace.size(); ++i) {
        const y::wvec2 p = origin + trace[i];
        tri_data.emplace_back(p[xx]);
        tri_data.emplace_back(p[yy]);
        origin_data.emplace_back(origin[xx]);
        origin_data.emplace_back(origin[yy]);
        range_data.emplace_back(light->full_range);
        range_data.emplace_back(light->falloff_range);
        layering_data.emplace_back(light->layer_value);
        if (!specular) {
          colour_data.emplace_back(light->colour[rr]);
          colour_data.emplace_back(light->colour[gg]);
          colour_data.emplace_back(light->colour[bb]);
          colour_data.emplace_back(light->colour[aa]);
        }
      }

      // Set up the indices.
      for (y::size i = 1; i < trace.size(); i += 2) {
        y::size prev = i - 1;
        y::size a = i;
        y::size b = (1 + i) % trace.size();
        y::size next = (2 + i) % trace.size();

        bool l_concave = trace[a].length_squared() <=
                         trace[prev].length_squared();
        bool r_concave = trace[b].length_squared() <=
                         trace[next].length_squared();
        y::size l = l_concave ? a : prev;
        y::size r = r_concave ? b : next;

        // Render the triangles: origin, l, r; l, r, b; a, b, l. Skip the
        // triangles which have no area (avoids artefacts).
        if (line_intersects_rect(y::wvec2(), trace[l], min, max) ||
            line_intersects_rect(y::wvec2(), trace[r], min, max) ||
            line_intersects_rect(trace[l], trace[r], min, max)) {
          element_data.emplace_back(origin_index);
          element_data.emplace_back(origin_index + 1 + l);
          element_data.emplace_back(origin_index + 1 + r);
        }
        if (!r_concave &&
            (line_intersects_rect(trace[l], trace[b], min, max) ||
             line_intersects_rect(trace[r], trace[b], min, max) ||
             line_intersects_rect(trace[l], trace[r], min, max))) {
          element_data.emplace_back(origin_index + 1 + l);
          element_data.emplace_back(origin_index + 1 + r);
          element_data.emplace_back(origin_index + 1 + b);
        }
        if (!l_concave &&
            (line_intersects_rect(trace[a], trace[l], min, max) ||
             line_intersects_rect(trace[b], trace[l], min, max) ||
             line_intersects_rect(trace[a], trace[b], min, max))) {
          element_data.emplace_back(origin_index + 1 + a);
          element_data.emplace_back(origin_index + 1 + b);
          element_data.emplace_back(origin_index + 1 + l);
        }
      }
    }
  }

  _tri_buffer->reupload_data(tri_data);
  _origin_buffer->reupload_data(origin_data);
  _range_buffer->reupload_data(range_data);
  if (!specular) {
    _colour_buffer->reupload_data(colour_data);
  }
  _layering_buffer->reupload_data(layering_data);
  _element_buffer->reupload_data(element_data);

  util.get_gl().enable_depth(false);
  util.get_gl().enable_blend(true, GL_SRC_ALPHA, GL_ONE);
  const GlProgram& program = specular ?
    *_point_light_specular_program : *_point_light_program;

  program.bind();
  program.bind_uniform("normalbuffer", normalbuffer);
  program.bind_attribute("pixels", *_tri_buffer);
  program.bind_attribute("origin", *_origin_buffer);
  program.bind_attribute("range", *_range_buffer);
  if (!specular) {
    program.bind_attribute("colour", *_colour_buffer);
  }
  program.bind_attribute("layer", *_layering_buffer);
  util.bind_pixel_uniforms(program);
  _element_buffer->draw_elements(GL_TRIANGLES, element_data.size());
}

Lighting::world_geometry::world_geometry()
{
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
                                     const OrderedGeometry& all_geometry) const
{
  // We could find only the vertices whose geometries intersect the circle
  // defined by origin and max_range, but that is way more expensive and
  // squares are easier anyway.
  y::set<y::wvec2> added_vertices;
  y::wvec2 min_bound = y::wvec2{-max_range, -max_range};
  y::wvec2 max_bound = y::wvec2{max_range, max_range};

  // See Collision::collider_move for details.
  for (y::size i = 0; i < all_geometry.buckets.size(); ++i) {
    y::int32 g_max = all_geometry.get_max_for_bucket(i);
    if (origin[xx] + min_bound[xx] >= g_max) {
      continue;
    }

    for (const Geometry& g : all_geometry.buckets[i]) {
      // Translate to origin.
      const y::wvec2 g_s = y::wvec2(g.start) - origin;
      const y::wvec2 g_e = y::wvec2(g.end) - origin;

      // Break by ordering.
      if (y::min(g_s[xx], g_e[xx]) >= max_bound[xx]) {
        break;
      }

      // Check intersection.
      if (!line_intersects_rect(g_s, g_e, min_bound, max_bound)) {
        continue;
      }

      // Exclude geometries which are defined in the wrong direction.
      if (g_s[yy] * g_e[xx] - g_s[xx] * g_e[yy] >= 0) {
        continue;
      }

      geometry_output.emplace_back(g_s, g_e);
      map_output[g_s].emplace_back(g_s, g_e);
      map_output[g_e].emplace_back(g_s, g_e);
    }
  }
  for (const auto& pair : map_output) {
    vertex_output.emplace_back(pair.first);
  }

  // Add corners of the max-range square. These aren't real vertices, but cause
  // the trace_light_geometry algorithm to stick to the outside of the square
  // when there is a period of rotation greater than pi / 2 containing no
  // vertices.
  vertex_output.emplace_back(y::wvec2{-max_range, -max_range});
  vertex_output.emplace_back(y::wvec2{max_range, -max_range});
  vertex_output.emplace_back(y::wvec2{-max_range, max_range});
  vertex_output.emplace_back(y::wvec2{max_range, max_range});
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
  typedef y::set<world_geometry, wg_hash> geometry_set;

  struct local {
    // Calculates point on geometry at the given angle vector.
    static y::wvec2 get_point_on_geometry(
        const y::wvec2& v, const world_geometry& geometry)
    {
      // Finds t such that g(t) = g.start + t * (g.end - g.start) has
      // g(t) cross v == 0.
      y::wvec2 g_vec = geometry.end - geometry.start;
      y::world d = v[xx] * g_vec[yy] - v[yy] * g_vec[xx];
      if (!d) {
        // Should have been excluded.
        return y::wvec2();
      }
      y::world t = (v[yy] * geometry.start[xx] -
                    v[xx] * geometry.start[yy]) / d;

      return geometry.start + t * g_vec;
    }

    // Calculates closest point and geometry.
    static y::wvec2 get_closest(
        world_geometry& closest_geometry_output, y::world max_range,
        const y::wvec2& v, const geometry_set& stack)
    {
      // If the stack is empty, use the max-range square.
      if (stack.empty()) {
        const y::wvec2 ul{-max_range, -max_range};
        const y::wvec2 ur{max_range, -max_range};
        const y::wvec2 dl{-max_range, max_range};
        const y::wvec2 dr{max_range, max_range};

        // Make sure we choose the edge in the direction of rotation at the
        // corners.
        if (v[xx] == v[yy]) {
          closest_geometry_output = v[xx] > 0 ?
              world_geometry(dr, dl) : world_geometry(ul, ur);
        }
        else if (v[xx] == -v[yy]) {
          closest_geometry_output = v[xx] > 0 ?
              world_geometry(ur, dr) : world_geometry(dl, ul);
        }
        else if (v[yy] > 0 && v[yy] >= y::abs(v[xx])) {
          closest_geometry_output = world_geometry(dr, dl);
        }
        else if (v[yy] < 0 && -v[yy] >= y::abs(v[xx])) {
          closest_geometry_output = world_geometry(ul, ur);
        }
        else if (v[xx] > 0 && v[xx] >= y::abs(v[yy])) {
          closest_geometry_output = world_geometry(ur, dr);
        }
        else if (v[xx] < 0 && -v[xx] >= y::abs(v[yy])) {
          closest_geometry_output = world_geometry(dl, ul);
        }
        else {
          closest_geometry_output.start = y::wvec2();
          closest_geometry_output.end = y::wvec2();
        }

        return get_point_on_geometry(v, closest_geometry_output);
      }

      const world_geometry* closest_geometry = y::null;
      y::wvec2 closest_point;

      bool first = true;
      y::world min_dist_sq = 0;
      for (const world_geometry& g : stack) {
        // Distance is defined by the intersection of the geometry with the
        // line from the origin to the current vertex.
        y::wvec2 point = get_point_on_geometry(v, g);
        y::world dist_sq = point.length_squared();

        if (first || dist_sq < min_dist_sq) {
          min_dist_sq = dist_sq;
          closest_geometry = &g;
          closest_point = point;
        }
        first = false;
      }

      closest_geometry_output = *closest_geometry;
      return closest_point;
    }
  };

  // Initialise the stack with geometry that intersects the line from the origin
  // to the first vertex (but doesn't start exactly on it). To make sure we get
  // only geometry crossing the positive half of the line and not the negative
  // half, make sure line is defined in correct direction.
  geometry_set stack;
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
  // the vertex is the start or end point of the geometry.
  world_geometry prev_closest_geometry;
  local::get_closest(prev_closest_geometry, max_range,
                     vertex_buffer[0], stack);
  // If stack is empty, make sure the first vertex gets added.
  bool add_first = stack.empty();

  for (y::size i = 0; i < vertex_buffer.size(); ++i) {
    const auto& v = vertex_buffer[i];

    // Add or remove from stack as appropriate.
    auto it = map.find(v);
    if (it != map.end()) {
      for (const world_geometry& g : it->second) {
        if (v == g.start) {
          stack.insert(g);
        }
        else {
          stack.erase(g);
        }
      }
    }

    // Determine whether this point represents a new angle or not. If we're on
    // the same line from the origin as the previous vertex, skip.
    if (i > 0) {
      const auto& prev = vertex_buffer[i - 1];
      if (v[xx] * prev[yy] - v[yy] - prev[xx] == 0) {
        continue;
      }
    }

    // Find the new closest geometry.
    world_geometry new_closest_geometry;
    y::wvec2 new_closest_point =
        local::get_closest(new_closest_geometry, max_range, v, stack);

    // When nothing has changed, skip (with special-case for empty stack at the
    // beginning).
    if (new_closest_geometry == prev_closest_geometry && !add_first) {
      continue;
    }
    add_first = false;

    // Add the two new points.
    y::wvec2 prev_closest_point =
        local::get_point_on_geometry(v, prev_closest_geometry);
    output.emplace_back(prev_closest_point);
    output.emplace_back(new_closest_point);

    // Store previous.
    prev_closest_geometry = new_closest_geometry;
  }
}

bool Lighting::line_intersects_rect(
    const y::wvec2& start, const y::wvec2& end,
    const y::wvec2& min, const y::wvec2& max)
{
  y::wvec2 line_min = y::min(start, end);
  y::wvec2 line_max = y::max(start, end);

  // Check bounds.
  if (!(line_min < max && line_max > min)) {
    return false;
  }

  // Check equation of line.
  if (start[xx] - end[xx] != 0) {
    y::world m = (end[yy] - start[yy]) / (end[xx] - start[xx]);
    y::world y_neg = end[yy] + m * (end[xx] + min[xx]);
    y::world y_pos = end[yy] + m * (end[xx] + max[xx]);

    if ((max[yy] < y_neg && max[yy] < y_pos) ||
        (min[yy] >= y_neg && min[yy] >= y_pos)) {
      return false;
    }
  }

  return true;
}
