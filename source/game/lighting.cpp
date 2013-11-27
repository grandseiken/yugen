#include "lighting.h"
#include "world.h"

#include "../render/gl_util.h"
#include "../render/util.h"

#include <algorithm>
#include <boost/functional/hash.hpp>

Light::Light(const Script& source)
  : source(source)
  , full_range(1.)
  , falloff_range(1.)
  , layer_value(0.)
  , colour{1.f, 1.f, 1.f, 1.f}
  , angle(0.)
  , aperture(y::pi)
{
  (void)source;
}

y::world Light::get_max_range() const
{
  return full_range + falloff_range;
}

y::wvec2 Light::get_origin(const y::wvec2& origin) const
{
  return is_planar() ? origin : origin + offset;
}

y::wvec2 Light::get_offset() const
{
  if (!is_planar()) {
    return offset;
  }

  // Need to swap the offset so that it is consistent relative to the normal.
  return offset.cross(normal_vec) >= 0 ? offset : -offset;
}

bool Light::is_planar() const
{
  return normal_vec != y::wvec2();
}

bool Light::overlaps_rect(const y::wvec2& origin,
                          const y::wvec2& min, const y::wvec2& max) const
{
  y::world max_range = get_max_range();
  y::wvec2 bound{max_range, max_range};
  if (!is_planar()) {
    return origin + bound >= min && origin - bound < max;
  }

  // Check rect against bounding-box of plane-light parallelogram. Could
  // probably be stricter, but this is simple and good enough.
  y::wvec2 a = origin - offset;
  y::wvec2 b = origin + offset;
  y::wvec2 c = a + normal_vec * max_range;
  y::wvec2 d = b + normal_vec * max_range;

  y::wvec2 light_min = y::min(y::min(a, b), y::min(c, d));
  y::wvec2 light_max = y::max(y::max(a, b), y::max(c, d));
  return light_max > min && light_min < max;
}

Lighting::Lighting(const WorldWindow& world, GlUtil& gl)
  : _world(world)
  , _gl(gl)
  , _light_program(gl.make_unique_program({
        "/shaders/light/light.v.glsl",
        "/shaders/light/light.f.glsl"}))
  , _light_specular_program(gl.make_unique_program({
        "/shaders/light/light_specular.v.glsl",
        "/shaders/light/light_specular.f.glsl"}))
  , _tri(gl.make_unique_buffer<GLfloat, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _origin(gl.make_unique_buffer<GLfloat, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _range(gl.make_unique_buffer<GLfloat, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _colour(gl.make_unique_buffer<GLfloat, 4>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _layering(gl.make_unique_buffer<GLfloat, 1>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _element_buffer(gl.make_unique_buffer<GLushort, 1>(
        GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW))
{
}

void Lighting::recalculate_traces(
    const y::wvec2& camera_min, const y::wvec2& camera_max)
{
  // Sorts points radially by angle from the origin point using a Graham
  // scan-like algorithm, starting at angle 0 (rightwards) and increasing
  // by angle.
  struct angular_order {
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

      y::world d = b.cross(a);
      // If d is zero, points are on same half-line, so fall back to distance
      // from the origin.
      return d < 0 ||
          (d == 0 && a.length_squared() < b.length_squared());
    }
  };

  // Sorts with respect to a given vector by projecting the two-dimensional
  // plane onto the line formed by the vector and sorting along this line.
  struct planar_order {
    y::wvec2 normal_vec;

    bool operator()(const y::wvec2& a, const y::wvec2& b) const
    {
      y::wvec2 plane_vec{normal_vec[yy], -normal_vec[xx]};
      y::world a_dot = a.dot(plane_vec);
      y::world b_dot = b.dot(plane_vec);

      // If dot-products are equal, points line up on projection, so fall back
      // to the signed distance from the line.
      return a_dot < b_dot ||
          (a_dot == b_dot && a.dot(normal_vec) < b.dot(normal_vec));
    }
  };

  // We could also limit max_range to camera bounds. However, this blows the
  // cache so it's not necessarily a good idea. In fact, it's certainly only
  // worth it for lights which are moving a lot (and also have a large max-
  // range), and so won't be cached anyway. Perhaps we can set this as a hint
  // somehow.
  //
  // Similarly, we key cone lights by position and max-range only, so they can
  // be rotated without needing recomputation. This means we have to compute
  // the entire 360-degree trace, so a hint could be added for cone lights which
  // move a lot and don't rotate.
  //
  // Only bother with this if performance actually becomes an issue.
  y::set<trace_key, trace_key_hash> trace_preserve;

  y::vector<y::wvec2> vertex_buffer;
  geometry_entry geometry_buffer;
  geometry_map map;
  angular_order angular;
  planar_order planar;

  source_list sources;
  get_sources(sources);
  for (const Script* s : sources) {
    for (const entry& light : get_list(*s)) {
      const y::wvec2 origin = light->get_origin(s->get_origin());

      // Skip if light doesn't overlap camera.
      if (!light->overlaps_rect(origin, camera_min, camera_max)) {
        continue;
      }

      trace_key key{origin, light->get_max_range(),
                    light->normal_vec, light->get_offset()};
      trace_preserve.insert(key);

      // If the result is cached, we don't need to recalculate it.
      if (_trace_results.find(key) != _trace_results.end()) {
        continue;
      }
      vertex_buffer.clear();
      geometry_buffer.clear();
      map.clear();

      // Find all the geometries that intersect the max-range square and their
      // vertices, translated respective to origin.
      get_relevant_geometry(vertex_buffer, geometry_buffer, map,
                            *light, origin, _world.get_geometry(),
                            light->is_planar());

      // Perform the appropriate vertex sort.
      if (light->is_planar()) {
        planar.normal_vec = light->normal_vec;
        std::sort(vertex_buffer.begin(), vertex_buffer.end(), planar);
      }
      else {
        std::sort(vertex_buffer.begin(), vertex_buffer.end(), angular);
      }

      // Trace the light geometry.
      // TODO: soften the shadows, somehow. I think the best approach is, for
      // each convex corner, to rotate the trace vector backwards slightly and
      // add another set of triangles from these. However if the new triangle
      // intersects geometry, we need to split it up (and split up the existing
      // line from the corner).
      trace_light_geometry(_trace_results[key], *light,
                           vertex_buffer, geometry_buffer, map,
                           light->is_planar());
    }
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

void Lighting::render_traces(
    RenderUtil& util,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  y::vector<RenderUtil::line> lines;

  for (const auto& pair : _trace_results) {
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

void Lighting::add_vertex(const y::wvec2& origin, const y::wvec2& trace,
                          const Light& light) const
{
  _tri.data.emplace_back(trace[xx]);
  _tri.data.emplace_back(trace[yy]);
  _origin.data.emplace_back(origin[xx]);
  _origin.data.emplace_back(origin[yy]);
  _range.data.emplace_back(light.full_range);
  _range.data.emplace_back(light.falloff_range);
  _layering.data.emplace_back(light.layer_value);
  _colour.data.emplace_back(light.colour[rr]);
  _colour.data.emplace_back(light.colour[gg]);
  _colour.data.emplace_back(light.colour[bb]);
  _colour.data.emplace_back(light.colour[aa]);
}

void Lighting::add_triangle(
    y::vector<GLushort>& element_data, const y::wvec2& min, const y::wvec2& max,
    y::size start_index, y::size a, y::size b, y::size c,
    const y::wvec2& at, const y::wvec2& bt, const y::wvec2& ct) const
{
  static const y::wvec2 o;
  // Check has area.
  if (a == b || a == c || a == b) {
    return;
  }
  // Check overlaps camera. The line-rect tests cover the case where the
  // triangle is inside or intersects the rectangle. To cover the case where the
  // rectangle is inside the triangle we check one vertex of the rectangle.
  bool ab_test = (bt - at).cross(min - at) < 0;
  bool bc_test = (ct - bt).cross(min - bt) < 0;
  bool ca_test = (at - ct).cross(min - ct) < 0;
  if (!y::line_intersects_rect(at, bt, min, max) &&
      !y::line_intersects_rect(at, ct, min, max) &&
      !y::line_intersects_rect(bt, ct, min, max) &&
      !(ab_test == bc_test && bc_test == ca_test)) {
    return;
  }
  element_data.emplace_back(start_index + a);
  element_data.emplace_back(start_index + b);
  element_data.emplace_back(start_index + c);
}

void Lighting::render_internal(
    RenderUtil& util, const GlFramebuffer& normalbuffer,
    const y::wvec2& camera_min, const y::wvec2& camera_max,
    bool specular) const
{
  if (!(util.get_resolution() >= y::ivec2())) {
    return;
  }
  _tri.data.clear();
  _origin.data.clear();
  _range.data.clear();
  _layering.data.clear();
  _colour.data.clear();
  _element_data.clear();

  source_list sources;
  get_sources(sources);
  for (const Script* s : sources) {
    for (const entry& light : get_list(*s)) {
      const y::wvec2 origin = light->get_origin(s->get_origin());

      trace_key key{origin, light->get_max_range(),
                    light->normal_vec, light->get_offset()};
      auto it = _trace_results.find(key);
      if (it == _trace_results.end() || !it->second.size()) {
        continue;
      }

      // If the light is conical, slice out the correct section.
      light_trace cone_trace;
      bool cone_light = !light->is_planar() && light->aperture < y::pi;
      const light_trace& trace = cone_light ? cone_trace : it->second;
      if (cone_light) {
        make_cone_trace(cone_trace, it->second, light->angle, light->aperture);
      }

      // Set up the vertex data and indices.
      if (light->is_planar()) {
        render_planar_internal(trace, *light, origin, camera_min, camera_max);
      }
      else {
        render_angular_internal(trace, *light, origin, camera_min, camera_max);
      }
    }
  }

  util.get_gl().enable_depth(true, GL_LESS);
  util.get_gl().enable_blend(true, GL_SRC_ALPHA, GL_ONE);
  const GlProgram& program = specular ?
    *_light_specular_program : *_light_program;

  program.bind();
  program.bind_uniform("normalbuffer", normalbuffer);
  program.bind_attribute("pixels", _tri.reupload());
  program.bind_attribute("origin", _origin.reupload());
  program.bind_attribute("range", _range.reupload());
  program.bind_attribute("layer", _layering.reupload());
  program.bind_attribute("colour", _colour.reupload());
  util.bind_pixel_uniforms(program);

  // It really shouldn't be necessary to use depth and draw lights one by one.
  // We should be able to draw them all at once. But for the life of me I can't
  // stop the light triangles from very occassionally overlapping and producing
  // artefacts, even though the triangles are formed with *exactly* the same
  // edges. I don't know what's going wrong.
  y::size n = _element_data.size();
  for (const auto& data : _element_data) {
    program.bind_uniform("depth", float(n--) / (1 + _element_data.size()));
    _element_buffer->reupload_data(data);
    _element_buffer->draw_elements(GL_TRIANGLES, data.size());
  }
}

void Lighting::render_angular_internal(
    const light_trace& trace, const Light& light, const y::wvec2& origin,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  // Arranging in a triangle fan causes tears in the triangles due to slight
  // inaccuracies, so we use a fan with three triangles per actual triangle
  // to make sure the edges line up exactly.
  y::size origin_index = _tri.data.size() / 2;

  // Set up the vertices.
  add_vertex(origin, origin, light);
  for (y::size i = 0; i < trace.size(); ++i) {
    add_vertex(origin, origin + trace[i], light);
  }
  const y::wvec2 min = camera_min - origin;
  const y::wvec2 max = camera_max - origin;

  // Set up the indices.
  _element_data.emplace_back();
  auto& light_element_data = *_element_data.rbegin();
  for (y::size i = 1; i < trace.size(); i += 2) {
    y::size prev = i - 1;
    y::size a = i;
    y::size b = (1 + i) % trace.size();
    y::size next = (2 + i) % trace.size();

    // Find the closer of the two possible vertices for left and right
    // (depending on whether each corner is a concave or convex one).
    y::size l = trace[a].length_squared() <=
                trace[prev].length_squared() ? a : prev;
    y::size r = trace[b].length_squared() <=
                trace[next].length_squared() ? b : next;

    // Render the triangles, using the configuration:
    // origin, l, r; l, r, b; a, b, l.
    // Triangles which have no area or do not overlap the camera are
    // skipped in the add_triangle function.
    add_triangle(light_element_data, min, max, origin_index,
                 0, 1 + l, 1 + r,
                 y::wvec2(), trace[l], trace[r]);
    add_triangle(light_element_data, min, max, origin_index,
                 1 + l, 1 + r, 1 + b,
                 trace[l], trace[r], trace[b]);
    add_triangle(light_element_data, min, max, origin_index,
                 1 + a, 1 + b, 1 + l,
                 trace[a], trace[b], trace[l]);
  }
}

void Lighting::render_planar_internal(
    const light_trace& trace, const Light& light, const y::wvec2& origin,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  // Similarly here we use up to 4 triangles to prevent tearing.
  y::size start_index = _tri.data.size() / 2;

  // Set up the vertices.
  y::wvec2 offset = light.get_offset();
  for (y::size i = 0; i < trace.size(); i += 2) {
    y::wvec2 on_plane = get_planar_point_on_geometry(
        light.normal_vec, trace[i], world_geometry(-offset, offset));
    add_vertex(origin + on_plane, origin + on_plane, light);
    add_vertex(origin + on_plane, origin + trace[i], light);
    add_vertex(origin + on_plane, origin + trace[1 + i], light);
  }

  const y::wvec2 min = camera_min - origin;
  const y::wvec2 max = camera_max - origin;

  // Set up the indices.
  _element_data.emplace_back();
  auto& light_element_data = *_element_data.rbegin();
  for (y::size i = 1; i < trace.size() - 2; i += 2) {
    y::size prev = i - 1;
    y::size a = i;
    y::size b = 1 + i;
    y::size next = 2 + i;

    y::wvec2 on_plane_l = get_planar_point_on_geometry(
        light.normal_vec, trace[a], world_geometry(-offset, offset));
    y::wvec2 on_plane_r = get_planar_point_on_geometry(
        light.normal_vec, trace[b], world_geometry(-offset, offset));

    y::size l = (trace[a] - on_plane_l).length_squared() <=
                (trace[prev] - on_plane_l).length_squared() ? a : prev;
    y::size r = (trace[b] - on_plane_r).length_squared() <=
                (trace[next] - on_plane_r).length_squared() ? b : next;

    // Render the triangles, using the configuration:
    // left on-plane, right on-plane, r; left on-plane, l, r; l, r, a; a, b, r.
    add_triangle(light_element_data, min, max, start_index,
                 3 * (l / 2),
                 3 * (r / 2),
                 1 + 3 * (r / 2) + (r % 2),
                 on_plane_l, on_plane_r, trace[r]);
    add_triangle(light_element_data, min, max, start_index,
                 3 * (l / 2),
                 1 + 3 * (l / 2) + (l % 2),
                 1 + 3 * (r / 2) + (r % 2),
                 on_plane_l, trace[l], trace[r]);
    add_triangle(light_element_data, min, max, start_index,
                 1 + 3 * (l / 2) + (l % 2),
                 1 + 3 * (r / 2) + (r % 2),
                 1 + 3 * (a / 2) + (a % 2),
                 trace[l], trace[r], trace[a]);
    add_triangle(light_element_data, min, max, start_index,
                 1 + 3 * (a / 2) + (a % 2),
                 1 + 3 * (b / 2) + (b % 2),
                 1 + 3 * (r / 2) + (r % 2),
                 trace[a], trace[b], trace[r]);
  }
}

bool Lighting::trace_key::operator==(const trace_key& key) const
{
  if (!(origin == key.origin && max_range == key.max_range &&
        normal_vec == key.normal_vec)) {
    return false;
  }
  return normal_vec == y::wvec2() || offset == key.offset;
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
  boost::hash_combine(seed, key.normal_vec[xx]);
  boost::hash_combine(seed, key.normal_vec[yy]);
  if (key.normal_vec != y::wvec2()) {
    boost::hash_combine(seed, key.offset[xx]);
    boost::hash_combine(seed, key.offset[yy]);
  }
  return seed;
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

y::size Lighting::world_geometry_hash::operator()(const world_geometry& g) const
{
  y::size seed = 0;
  boost::hash_combine(seed, g.start[xx]);
  boost::hash_combine(seed, g.start[yy]);
  boost::hash_combine(seed, g.end[xx]);
  boost::hash_combine(seed, g.end[yy]);
  return seed;
}

void Lighting::get_relevant_geometry(
    y::vector<y::wvec2>& vertex_output, geometry_entry& geometry_output,
    geometry_map& map_output, const Light& light, const y::wvec2& origin,
    const WorldGeometry::geometry_hash& all_geometry, bool planar)
{
  if (planar) {
    get_planar_relevant_geometry(vertex_output, geometry_output, map_output,
                                 light, origin, all_geometry);
  }
  else {
    get_angular_relevant_geometry(vertex_output, geometry_output, map_output,
                                  light, origin, all_geometry);
  }
}

void Lighting::get_angular_relevant_geometry(
    y::vector<y::wvec2>& vertex_output, geometry_entry& geometry_output,
    geometry_map& map_output, const Light& light, const y::wvec2& origin,
    const WorldGeometry::geometry_hash& all_geometry)
{
  // We could find only the vertices whose geometries intersect the circle
  // defined by origin and max_range, but that is way more expensive and
  // squares are easier anyway.
  y::world max_range = light.get_max_range();
  y::wvec2 bound = y::wvec2{max_range, max_range};

  // See Collision::collider_move for details.
  for (auto it = all_geometry.search(origin - bound,
                                     origin + bound); it; ++it) {
    // Translate to origin.
    const y::wvec2 g_s = y::wvec2(it->start) - origin;
    const y::wvec2 g_e = y::wvec2(it->end) - origin;

    // Check intersection.
    if (!y::line_intersects_rect(g_s, g_e, -bound, bound)) {
      continue;
    }

    // Exclude geometries which are defined in the wrong direction, that is,
    // check whether the line is going clockwise or anticlockwise around the
    // origin.
    if (g_e.cross(g_s) >= 0) {
      continue;
    }

    // We allow lights to shine in from outside the active window.
    if (it->external) {
      continue;
    }

    geometry_output.emplace_back(g_s, g_e);
    map_output[g_s].emplace_back(g_s, g_e);
    map_output[g_e].emplace_back(g_s, g_e);
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

void Lighting::get_planar_relevant_geometry(
    y::vector<y::wvec2>& vertex_output, geometry_entry& geometry_output,
    geometry_map& map_output, const Light& light, const y::wvec2& origin,
    const WorldGeometry::geometry_hash& all_geometry)
{
  // We find all the vertices whose geometries intersect the bounding box of the
  // plane-light parallelogram.
  y::wvec2 offset = light.get_offset();
  y::wvec2 v = light.get_max_range() * light.normal_vec;

  y::wvec2 min_bound = y::min(y::min(-offset, offset),
                              y::min(v - offset, v + offset));
  y::wvec2 max_bound = y::max(y::max(-offset, offset),
                              y::max(v - offset, v + offset));

  // See above for details.
  for (auto it = all_geometry.search(origin + min_bound,
                                     origin + max_bound); it; ++it) {
    const y::wvec2 g_s = y::wvec2(it->start) - origin;
    const y::wvec2 g_e = y::wvec2(it->end) - origin;

    if (!y::line_intersects_rect(g_s, g_e, min_bound, max_bound)) {
      continue;
    }

    // Exclude geometries which cross the light angle in the wrong direction.
    if ((g_e - g_s).cross(v) >= 0) {
      continue;
    }

    if (it->external) {
      continue;
    }

    geometry_output.emplace_back(g_s, g_e);
    map_output[g_s].emplace_back(g_s, g_e);
    map_output[g_e].emplace_back(g_s, g_e);
  }
  for (const auto& pair : map_output) {
    vertex_output.emplace_back(pair.first);
  }

  vertex_output.emplace_back(y::wvec2{v - offset});
  vertex_output.emplace_back(y::wvec2{v + offset});
}

y::wvec2 Lighting::get_angular_point_on_geometry(
    const y::wvec2& v, const world_geometry& geometry)
{
  // Calculates point on geometry at the given angle vector. Finds t such that
  // g(t) = g.start + t * (g.end - g.start) has g(t) cross v == 0.
  y::wvec2 g_vec = geometry.end - geometry.start;
  y::world d = v.cross(g_vec);
  if (!d) {
    // Should have been excluded.
    return y::wvec2();
  }
  y::world t = geometry.start.cross(v) / d;

  return geometry.start + t * g_vec;
}

y::wvec2 Lighting::get_planar_point_on_geometry(
    const y::wvec2& normal_vec, const y::wvec2& v,
    const world_geometry& geometry)
{
  const y::wvec2 g_vec = geometry.end - geometry.start;
  const y::world d = g_vec.cross(normal_vec);
  if (!d) {
    // Normal in same direction as geometry. Should have been excluded.
    return y::wvec2();
  }
  y::world t = g_vec.cross(geometry.start - v) / d;

  return v + t * normal_vec;
}

void Lighting::trace_light_geometry(light_trace& output, const Light& light,
                                    const y::vector<y::wvec2>& vertex_buffer,
                                    const geometry_entry& geometry_buffer,
                                    const geometry_map& map, bool planar)
{
  if (planar) {
    trace_planar_light_geometry(output, light,
                                vertex_buffer, geometry_buffer, map);
  }
  else {
    trace_angular_light_geometry(output, light,
                                 vertex_buffer, geometry_buffer, map);
  }
}

void Lighting::trace_angular_light_geometry(
    light_trace& output, const Light& light,
    const y::vector<y::wvec2>& vertex_buffer,
    const geometry_entry& geometry_buffer,
    const geometry_map& map)
{
  struct local {
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

        return get_angular_point_on_geometry(v, closest_geometry_output);
      }

      const world_geometry* closest_geometry = y::null;
      y::wvec2 closest_point;

      bool first = true;
      y::world min_dist_sq = 0;
      for (const world_geometry& g : stack) {
        // Distance is defined by the intersection of the geometry with the
        // line from the origin to the current vertex.
        y::wvec2 point = get_angular_point_on_geometry(v, g);
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
    // If d_e < 0 && d_s > 0 then line crosses the negative half.
    if (first_vec.cross(g.start) < 0 && first_vec.cross(g.end) >= 0) {
      stack.insert(g);
    }
  }

  // Algorithm: loop through the vertices. When it's the end of a geometry line
  // in the stack, relative to the angular sweep direction, remove from the
  // stack. When it's the start, add to the stack. Since we've excluded geometry
  // defined opposite the sweep direction, this corresponds exactly to whether
  // the vertex is the start or end point of the geometry.
  world_geometry prev_closest_geometry;
  local::get_closest(prev_closest_geometry, light.get_max_range(),
                     first_vec, stack);
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
    // the same line from the origin as the next vertex, skip.
    if (i < vertex_buffer.size() - 1) {
      const auto& next = vertex_buffer[1 + i];
      if (v.cross(next) == 0) {
        continue;
      }
    }

    // Find the new closest geometry.
    world_geometry new_closest_geometry;
    y::wvec2 new_closest_point =
        local::get_closest(new_closest_geometry, light.get_max_range(),
                           v, stack);

    // When nothing has changed, skip (with special-case for empty stack at the
    // beginning).
    if (new_closest_geometry == prev_closest_geometry && !add_first) {
      continue;
    }
    add_first = false;

    // Add the two new points.
    y::wvec2 prev_closest_point =
        get_angular_point_on_geometry(v, prev_closest_geometry);
    output.emplace_back(prev_closest_point);
    output.emplace_back(new_closest_point);

    // Store previous.
    prev_closest_geometry = new_closest_geometry;
  }
}

void Lighting::trace_planar_light_geometry(
    light_trace& output, const Light& light,
    const y::vector<y::wvec2>& vertex_buffer,
    const geometry_entry& geometry_buffer,
    const geometry_map& map)
{
  struct local {
    // Calculates closest point and geometry.
    static y::wvec2 get_closest(
        world_geometry& closest_geometry_output, const Light& light,
        const y::wvec2& v, const geometry_set& stack)
    {
      const world_geometry* closest_geometry = y::null;
      y::wvec2 closest_point;
      // Point on light plane for comparison.
      const y::wvec2 plane_point = get_planar_point_on_geometry(
          light.normal_vec, v,
          world_geometry(-light.get_offset(), light.get_offset()));

      bool first = true;
      y::world min_dist_sq = 0;
      for (const world_geometry& g : stack) {
        y::wvec2 point = get_planar_point_on_geometry(light.normal_vec, v, g);
        y::world dist_sq = (point - plane_point).length_squared();

        if (first || dist_sq < min_dist_sq) {
          min_dist_sq = dist_sq;
          closest_geometry = &g;
          closest_point = point;
        }
        first = false;
      }

      // If the stack is empty (or all points were on the wrong side), use the
      // max-range plane.
      if (first) {
        closest_geometry_output.end =
            light.normal_vec * light.get_max_range() - light.get_offset();
        closest_geometry_output.start =
            light.normal_vec * light.get_max_range() + light.get_offset();

        return get_planar_point_on_geometry(
            light.normal_vec, v, closest_geometry_output);
      }

      closest_geometry_output = *closest_geometry;
      return closest_point;
    }
  };

  // The strategy is very similar to angular tracing, but in coordinates with
  // respect to basis defined by light angle rather than angular coordinates.
  // Currently, this will not work correctly if any of the geometry lines cross
  // the light plane; this would be solved by adding the intersections to the
  // vertex list in get_planar_relevant_geometry and limiting points behind the
  // light plane.
  const y::wvec2 plane_vec{light.normal_vec[yy], -light.normal_vec[xx]};

  // Initialise the stack with geometry that intersects the line from the first
  // vertex towards the light angle (but doesn't start exactly on it).
  geometry_set stack;
  const y::wvec2& first_vec = vertex_buffer[0];
  for (const world_geometry& g : geometry_buffer) {
    // Lines crossing in the opposite direction have already been excluded.
    if ((g.start - first_vec).cross(light.normal_vec) < 0 &&
        (g.end - first_vec).cross(light.normal_vec) >= 0) {
      stack.insert(g);
    }
  }

  // Algorithm: same as the angular case, but sweeping along a plane. See above
  // for details.
  world_geometry prev_closest_geometry;
  local::get_closest(prev_closest_geometry, light, first_vec, stack);
  bool add_first = stack.empty();

  for (y::size i = 0; i < vertex_buffer.size(); ++i) {
    const auto& v = vertex_buffer[i];

    auto it = map.find(v);
    if (it != map.end()) {
      for (const world_geometry& g : it->second) {
        if (v == g.end) {
          stack.insert(g);
        }
        else {
          stack.erase(g);
        }
      }
    }

    // Determine whether this point represents a new sweep position or not. If
    // we're on the same direction line from plane as next vertex, skip.
    if (i < vertex_buffer.size() - 1) {
      const auto& next = vertex_buffer[1 + i];
      if (v.dot(plane_vec) == next.dot(plane_vec)) {
        continue;
      }
    }

    world_geometry new_closest_geometry;
    y::wvec2 new_closest_point =
        local::get_closest(new_closest_geometry, light, v, stack);

    bool add_last = i == vertex_buffer.size() - 1 && stack.empty();
    if (new_closest_geometry == prev_closest_geometry &&
        !add_first && !add_last) {
      continue;
    }
    add_first = false;

    y::wvec2 prev_closest_point =
        get_planar_point_on_geometry(
            light.normal_vec, v, prev_closest_geometry);
    output.emplace_back(prev_closest_point);
    output.emplace_back(new_closest_point);

    prev_closest_geometry = new_closest_geometry;
  }
}

void Lighting::make_cone_trace(light_trace& output, const light_trace& trace,
                               y::world angle, y::world aperture)
{
  y::world min_angle = y::angle(angle - aperture);
  y::world max_angle = y::angle(angle + aperture);
  y::wvec2 min = y::wvec2::from_angle(min_angle);
  y::wvec2 max = y::wvec2::from_angle(max_angle);

  // We need to re-order the trace so that it doesn't have a big gap in the
  // middle; do this by detecting straight runs of points in the cone.
  bool straight = false;
  y::vector<light_trace> straight_traces;

  y::size min_index = 0;
  y::size max_index = 0;

  y::size i = 0;
  for (; i < trace.size(); i += 2) {
    const y::wvec2& v = trace[i];
    const y::wvec2& w = trace[1 + i];
    const y::wvec2& v2 = trace[(2 + i) % trace.size()];

    y::world min_check = v.cross(min);
    y::world max_check = v.cross(max);

    // Find start and end indices of the conical section.
    if (min_check >= 0 && v2.cross(min) < 0) {
      min_index = i;
    }
    if (max_check >= 0 && v2.cross(max) < 0) {
      max_index = i;
    }

    // Conical section is exactly the intersection of the half-planes defined
    // by the minimum and maximum angles.
    if (aperture > y::pi / 2 ? min_check < 0 || max_check >= 0 :
                               min_check < 0 && max_check >= 0) {
      if (!straight) {
        straight = true;
        straight_traces.emplace_back();
      }
      straight_traces.rbegin()->emplace_back(v);
      straight_traces.rbegin()->emplace_back(w);
    }
    else {
      straight = false;
    }
  }

  // Assemble the trace from the straight-runs, with extra pairs at the start
  // and end.
  world_geometry min_cross(trace[1 + min_index],
                           trace[(2 + min_index) % trace.size()]);
  world_geometry max_cross(trace[1 + max_index],
                           trace[(2 + max_index) % trace.size()]);
  output.emplace_back();
  output.emplace_back(get_angular_point_on_geometry(min, min_cross));
  for (auto it = straight_traces.rbegin(); it != straight_traces.rend(); ++it) {
    output.insert(output.end(), it->begin(), it->end());
  }
  output.emplace_back(get_angular_point_on_geometry(max, max_cross));
  output.emplace_back();
}

