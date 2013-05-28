#include "collision.h"
#include "render_util.h"
#include "world.h"

// Must be kept in sync with collide.lua.
enum CollideMaskReserved {
  COLLIDE_RESV_NONE = 0,
  COLLIDE_RESV_WORLD = 1,
  COLLIDE_RESV_0 = 2,
  COLLIDE_RESV_1 = 4,
  COLLIDE_RESV_2 = 8,
};

Body::Body(const Script& source)
  : source(source)
  , collide_mask(COLLIDE_RESV_NONE)
{
}

y::wvec2 Body::get_min() const
{
  y::wvec2 min = offset - size / 2;
  y::wvec2 max = offset + size / 2;
  y::world size = y::max(y::abs(min), y::abs(max)).length();
  return {-size, -size};
}

y::wvec2 Body::get_max() const
{
  y::wvec2 min = offset - size / 2;
  y::wvec2 max = offset + size / 2;
  y::world size = y::max(y::abs(min), y::abs(max)).length();
  return {size, size};
}

void Body::get_vertices(y::vector<y::wvec2>& output,
                        const y::wvec2& origin, y::world rotation) const
{
  // Rotation matrix.
  // TODO: if we need to use this elsewhere, standardise it somewhere.
  y::wvec2 row_0(cos(rotation), -sin(rotation));
  y::wvec2 row_1(sin(rotation), cos(rotation));

  y::wvec2 dr = offset + size / 2;
  y::wvec2 ur = offset + y::wvec2{size[xx], -size[yy]} / 2;
  y::wvec2 dl = offset + y::wvec2{-size[xx], size[yy]} / 2;
  y::wvec2 ul = offset - size / 2;

  output.emplace_back(origin + y::wvec2{ul.dot(row_0), ul.dot(row_1)});
  output.emplace_back(origin + y::wvec2{ur.dot(row_0), ur.dot(row_1)});
  output.emplace_back(origin + y::wvec2{dr.dot(row_0), dr.dot(row_1)});
  output.emplace_back(origin + y::wvec2{dl.dot(row_0), dl.dot(row_1)});
}

Collision::Collision(const WorldWindow& world)
  : _world(world)
{
}

void Collision::render(
    RenderUtil& util,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  // TODO: consider batching all these line draws?
  y::size i = 0;
  for (const OrderedBucket& bucket : _world.get_geometry().buckets) {
    for (const Geometry& g : bucket) {
      const y::fvec4 c = ++i % 2 ? y::fvec4(1.f, 0.f, 0.f, .5f) :
                                   y::fvec4(0.f, 1.f, 0.f, .5f);

      const y::ivec2 max = y::max(g.start, g.end);
      const y::ivec2 min = y::min(g.start, g.end);
      if (y::wvec2(max) >= camera_min && y::wvec2(min) < camera_max) {
        util.render_line(y::fvec2(g.start), y::fvec2(g.end), c);
      }
    }
  }

  const y::fvec4 c{0.f, 0.f, 1.f, .5f};
  source_list sources;
  get_sources(sources);
  for (const Script* s : sources) {
    y::vector<y::wvec2> vertices;

    for (const entry& b : get_list(*s)) {
      if (!b->collide_mask) {
        continue;
      }

      y::wvec2 min = s->get_origin() + b->get_min();
      y::wvec2 max = s->get_origin() + b->get_max();
      if (!(max >= camera_min && min < camera_max)) {
        continue;
      }

      vertices.clear();
      b->get_vertices(vertices, s->get_origin(), s->get_rotation());
      for (y::size i = 0; i < vertices.size(); ++i) {
        util.render_line(y::fvec2(vertices[i]),
                         y::fvec2(vertices[(i + 1) % vertices.size()]), c);
      }
    }
  }
}

void Collision::collider_move(Script& source, const y::wvec2& move) const
{
  const entry_list& bodies = get_list(source);
  if (bodies.empty() || move == y::wvec2()) {
    source.set_origin(source.get_origin() + move);
    return;
  }

  // So far, we only collide Bodies with world geometry.
  // TODO: collide bodies with bodies. When we do that: to avoid order-dependent
  // edge-cases (e.g. player standing on platform moving downwards), will need
  // to store per-frame list of things which tried to move but were blocked by
  // bodies. If the blocker moves away, try again to move all the things which
  // were blocked by it.
  const OrderedGeometry& geometry = _world.get_geometry();

  // Bounding boxes of the source Bodies.
  y::wvec2 min_bound =
      source.get_origin() + get_min(bodies, COLLIDE_RESV_WORLD);
  min_bound = y::min(min_bound, move + min_bound);
  y::wvec2 max_bound =
      source.get_origin() + get_max(bodies, COLLIDE_RESV_WORLD);
  max_bound = y::max(max_bound, move + max_bound);

  y::world min_ratio = 1;
  // Eliminating geometry by bucket and order depends heavily on structure of
  // OrderedGeometry.
  for (y::size i = 0; i < geometry.buckets.size(); ++i) {
    // Skip buckets we're completely outside. If this gets too slow, we can
    // bucket by both x and y.
    y::int32 g_max = geometry.get_max_for_bucket(i);
    if (min_bound[xx] >= g_max) {
      continue;
    }

    // By reversing the nesting of these loops we could skip geometry on a
    // per-body basis. Incurs extra overhead though, and since most things will
    // have one body it's unlikely to be worth it.
    for (const Geometry& g : geometry.buckets[i]) {
      y::wvec2 g_min = y::wvec2(y::min(g.start, g.end));
      y::wvec2 g_max = y::wvec2(y::max(g.start, g.end));
      // By ordering, once we see a high enough minimum we can break.
      if (g_min[xx] >= max_bound[xx]) {
        break;
      }

      world_geometry wg{y::wvec2(g.start), y::wvec2(g.end)};
      // Skip geometry which is defined opposite the direction of movement, and
      // geometry for which a general bounding-box check fails.
      y::wvec2 g_vec = wg.end - wg.start;
      y::wvec2 normal{g_vec[yy], -g_vec[xx]};
      if (normal.dot(-move) <= 0 ||
          !(g_min < max_bound && g_max > min_bound)) {
        continue;
      }

      // Now: project each relevant (depending on direction of movement) vertex
      // of each body by the movement vector to form a line. Find the minimum
      // blocking ratio among each intersection of the geometry with these, and
      // and also any endpoint of geometry contained within the projection of
      // the entire shape.
      // The pleasing symmetry is that checking the geometry endpoints is
      // exactly the reverse of the same process, i.e., project the geometry
      // backwards by the movement vector and take the minimum blocking ratio
      // among intersections with the original shape.
      y::vector<y::wvec2> vertices;
      y::vector<world_geometry> geometries;

      for (const auto& pointer : bodies) {
        const Body& b = *pointer;
        if (!(b.collide_mask & COLLIDE_RESV_WORLD)) {
          continue;
        }
        vertices.clear();
        geometries.clear();

        b.get_vertices(vertices,
                       source.get_origin(), source.get_rotation());
        get_geometries(geometries, vertices);

        min_ratio = y::min(min_ratio, get_projection_ratio(wg, vertices, move));
        min_ratio = y::min(min_ratio, get_projection_ratio(
                        geometries, {wg.start, wg.end}, -move));
      }
    }
  }
  // Limit the movement to the minimum blocking ratio (i.e., least 0 <= t <= 1
  // such that moving by t * move is blocked), and recurse in any free unblocked
  // direction.
  source.set_origin(min_ratio * move + source.get_origin());
  // TODO: (optionally) recurse?
}

void Collision::collider_rotate(Script& source, y::world rotate) const
{
  const entry_list& bodies = get_list(source);
  if (bodies.empty() || rotate == 0.) {
    source.set_rotation(y::angle(rotate + source.get_rotation()));
    return;
  }

  // TODO: collide bodies with bodies when rotating.
  const OrderedGeometry& geometry = _world.get_geometry();

  // Bounding boxes of the source Bodies.
  y::wvec2 min_bound =
      source.get_origin() + get_min(bodies, COLLIDE_RESV_WORLD);
  y::wvec2 max_bound =
      source.get_origin() + get_max(bodies, COLLIDE_RESV_WORLD);

  y::world limiting_rotation = y::abs(rotate);
  // See collider_move for details. Can we unify this into a common function?
  for (y::size i = 0; i < geometry.buckets.size(); ++i) {
    y::int32 g_max = geometry.get_max_for_bucket(i);
    if (min_bound[xx] >= g_max) {
      continue;
    }

    for (const Geometry& g : geometry.buckets[i]) {
      y::wvec2 g_min = y::wvec2(y::min(g.start, g.end));
      y::wvec2 g_max = y::wvec2(y::max(g.start, g.end));
      if (g_min[xx] >= max_bound[xx]) {
        break;
      }
      // We can't skip geometry defined in opposite direction in the outer loop,
      // since direction depends on body involved. That happens in
      // get_arc_projection.
      if (!(g_min < max_bound && g_max > min_bound)) {
        continue;
      }

      // Very similar strategy to collider move, except we are projecting points
      // along arcs of circles defined by the distance to the Script's origin.
      world_geometry wg{y::wvec2(g.start), y::wvec2(g.end)};
      y::vector<y::wvec2> vertices;
      y::vector<world_geometry> geometries;

      for (const auto& pointer : bodies) {
        const Body& b = *pointer;
        if (!(b.collide_mask & COLLIDE_RESV_WORLD)) {
          continue;
        }
        vertices.clear();
        geometries.clear();

        b.get_vertices(vertices,
                       source.get_origin(), source.get_rotation());
        get_geometries(geometries, vertices);

        limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                                wg, vertices, source.get_origin(), rotate));
        limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                                geometries, {wg.start, wg.end},
                                source.get_origin(), -rotate));
      }
    }
  }
  // Rotate.
  source.set_rotation(y::angle(
      limiting_rotation * (rotate > 0 ? 1 : -1) + source.get_rotation()));
}

bool Collision::body_check(const Script& source, const Body& body,
                           y::int32 collide_mask) const
{
  if (!(collide_mask & COLLIDE_RESV_WORLD)) {
    return false;
  }
  const OrderedGeometry& geometry = _world.get_geometry();
  y::wvec2 min_bound = source.get_origin() + body.get_min();
  y::wvec2 max_bound = source.get_origin() + body.get_max();

  y::vector<y::wvec2> vertices;
  y::vector<world_geometry> geometries;

  body.get_vertices(vertices,
                    source.get_origin(), source.get_rotation());
  get_geometries(geometries, vertices);

  // See collider_move for details.
  for (y::size i = 0; i < geometry.buckets.size(); ++i) {
    y::int32 g_max = geometry.get_max_for_bucket(i);
    if (min_bound[xx] >= g_max) {
      continue;
    }

    for (const Geometry& g : geometry.buckets[i]) {
      y::wvec2 g_min = y::wvec2(y::min(g.start, g.end));
      y::wvec2 g_max = y::wvec2(y::max(g.start, g.end));
      if (g_min[xx] >= max_bound[xx]) {
        break;
      }
      if (!(g_min < max_bound && g_max > min_bound)) {
        continue;
      }

      if (has_intersection(geometries, world_geometry{y::wvec2(g.start),
                                                      y::wvec2(g.end)})) {
        return true;
      }
    }
  }

  return false;
}

y::world Collision::get_projection_ratio(
    const y::vector<world_geometry>& geometry,
    const y::vector<y::wvec2>& vertices, const y::wvec2& move) const
{
  y::world min_ratio = 2;
  for (const world_geometry& g : geometry) {
    for (const y::wvec2& v : vertices) {
      min_ratio = y::min(min_ratio, get_projection_ratio(g, v, move));
    }
  }
  return min_ratio;
}

y::world Collision::get_projection_ratio(
    const world_geometry& geometry,
    const y::vector<y::wvec2>& vertices, const y::wvec2& move) const
{
  y::world min_ratio = 2;
  for (const y::wvec2& v : vertices) {
    min_ratio = y::min(min_ratio, get_projection_ratio(geometry, v, move));
  }
  return min_ratio;
}

y::world Collision::get_projection_ratio(
    const world_geometry& geometry,
    const y::wvec2& vertex, const y::wvec2& move) const
{
  world_geometry v{vertex, move + vertex};
  const world_geometry& g = geometry;

  // Skip geometry in the wrong direction.
  y::wvec2 g_vec = g.end - g.start;
  y::wvec2 normal{g_vec[yy], -g_vec[xx]};
  if (normal.dot(-move) <= 0) {
    return 2;
  }

  // Equations of lines (for t, u in [0, 1]):
  // v(t) = v.start + t * (v.end - v.start)
  // g(u) = g.start + u * (g.end - g.start)
  // Finds t, u such that v(t) = g(u).
  y::world denominator =
      (g.end[xx] - g.start[xx]) * (v.end[yy] - v.start[yy]) -
      (g.end[yy] - g.start[yy]) * (v.end[xx] - v.start[xx]);

  // Lines are parallel or coincident.
  if (!denominator) {
    return 2;
  }

  y::world t = (
      (g.end[xx] - g.start[xx]) * (v.start[yy] - g.start[yy]) -
      (g.end[yy] - g.start[yy]) * (v.start[xx] - g.start[xx])) / -denominator;
  y::world u = (
      (v.end[xx] - v.start[xx]) * (g.start[yy] - v.start[yy]) -
      (v.end[yy] - v.start[yy]) * (g.start[xx] - v.start[xx])) / denominator;

  y::world tolerance = 1.0 / 2048;
  // Lines intersect outside of the segments.
  if (t < -tolerance || t > 1 || u < 0 || u > 1) {
    return 2;
  }
  return t;
}

y::world Collision::get_arc_projection(
    const y::vector<world_geometry>& geometry,
    const y::vector<y::wvec2>& vertices,
    const y::wvec2& origin, y::world rotation) const
{
  y::world limiting_rotation = y::abs(rotation);
  for (const world_geometry& g : geometry) {
    for (const y::wvec2& v : vertices) {
      limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                              g, v, origin, rotation));
    }
  }
  return limiting_rotation;
}

y::world Collision::get_arc_projection(
    const world_geometry& geometry,
    const y::vector<y::wvec2>& vertices,
    const y::wvec2& origin, y::world rotation) const
{
  y::world limiting_rotation = y::abs(rotation);
  for (const y::wvec2& v : vertices) {
    limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                            geometry, v, origin, rotation));
  }
  return limiting_rotation;
}

y::world Collision::get_arc_projection(
    const world_geometry& geometry,
    const y::wvec2& vertex,
    const y::wvec2& origin, y::world rotation) const
{
  const world_geometry& g = geometry;

  // Equation of line (for t in [0, 1]):
  // g(t) = g.start + t * (g.end - g.start)
  // Equation of circle:
  // |x - origin| = |vertex - origin|
  // Finds all t such that |g(t) - origin| = |vertex - origin|.
  // These are the roots of the equation a * t^2 + b * t + c = 0.
  y::world r_sq = (vertex - origin).length_squared();
  y::wvec2 g_vec = (g.end - g.start);

  y::world a = g_vec.length_squared();
  y::world b = 2 * (g.start.dot(g.end) + g.start.dot(origin) -
                    g.end.dot(origin) - g.start.length_squared());
  y::world c = (g.start - origin).length_squared() - r_sq;

  y::world sqrtand = b * b - 4 * a * c;

  // If no roots the line and circle do not meet. If one root they touch at a
  // single point, so also cannot be blocking. If a is zero the line is a point.
  if (sqrtand <= 0 || a == 0) {
    return y::abs(rotation);
  }

  y::world sqrted = sqrt(sqrtand);
  y::world t_0 = (-b + sqrted) / (2 * a);
  y::world t_1 = (-b - sqrted) / (2 * a);

  // If any root t satisfies 0 <= t <= 1 then it is a block of the circle by the
  // line, so find the angle and limit.
  struct local {
    static y::world limit(y::world t, y::world rotation,
                          y::world initial_angle,
                          const y::wvec2& g_start, const y::wvec2& g_vec,
                          const y::wvec2& origin)
    {
      static const y::world tolerance = 1.0 / 4096;
      // Impact outside the line segment.
      if (t < 0 || t > 1) {
        return y::abs(rotation);
      }
      y::wvec2 impact_rel = g_start + t * g_vec - origin;
      // Skip if the collision is opposite the direction the line is defined in.
      // This is done by forming a line from the origin in the direction of the
      // collision line's normal, and checking which side of it the impact point
      // is on.
      y::world signed_distance = g_vec.dot(impact_rel);
      if ((rotation > 0) != (signed_distance > 0)) {
        return y::abs(rotation);
      }

      y::world limiting_angle = impact_rel.angle();
      // Finds the (absolute) limiting rotation in the direction of rotation
      // (so it needs to be made negative again if the rotation is negative).
      y::world limiting_rotation =
          (rotation > 0 ? limiting_angle - initial_angle :
                          initial_angle - limiting_angle) +
          ((rotation > 0) != (limiting_angle > initial_angle) ? 2 * y::pi : 0);
      // Because of trigonometric inaccuracies and the fact that if we are a
      // tiny bit out in the wrong direction the rotation will not be blocked
      // at all, it's best to have a small amount of tolerance.
      if (2 * y::pi - limiting_rotation < tolerance) {
        return limiting_rotation - 2 * y::pi;
      }
      return limiting_rotation;
    }
  };
  y::world initial_angle = (vertex - origin).angle();
  y::world limiting_rotation = y::min(
      local::limit(t_0, rotation, initial_angle, g.start, g_vec, origin),
      local::limit(t_1, rotation, initial_angle, g.start, g_vec, origin));
  return limiting_rotation;
}

bool Collision::has_intersection(const y::vector<world_geometry>& a,
                                 const world_geometry& b) const
{
  for (const world_geometry& g : a) {
    if (has_intersection(g, b)) {
      return true;
    }
  }
  return false;
}

bool Collision::has_intersection(const world_geometry& a,
                                 const world_geometry& b) const
{
  return get_projection_ratio(a, b.start, b.end - b.start) <= 1;
}

// TODO: not having collide_mask here makes collision different?
// Since this should just be an optimisation that's odd; investigate.
y::wvec2 Collision::get_min(const entry_list& bodies,
                            y::int32 collide_mask) const
{
  y::wvec2 min;
  bool first = true;
  for (const entry& e : bodies) {
    if (!(e->collide_mask & collide_mask)) {
      continue;
    }
    min = first ? e->get_min() : y::min(min, e->get_min());
    first = false;
  }
  return min;
}

y::wvec2 Collision::get_max(const entry_list& bodies,
                            y::int32 collide_mask) const
{
  y::wvec2 max;
  bool first = true;
  for (const entry& e : bodies) {
    if (!(e->collide_mask & collide_mask)) {
      continue;
    }
    max = first ? e->get_max() : y::max(max, e->get_max());
    first = false;
  }
  return max;
}

void Collision::get_geometries(y::vector<world_geometry>& output,
                               const y::vector<y::wvec2>& vertices) const
{
  for (y::size i = 0; i < vertices.size(); ++i) {
    output.push_back({vertices[i], vertices[(i + 1) % vertices.size()]});
  }
}
