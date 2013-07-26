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

Body::bounds Body::get_bounds(const y::wvec2& origin,
                              y::world rotation) const
{
  y::vector<y::wvec2> vertices;
  get_vertices(vertices, origin, rotation);

  y::wvec2 min;
  y::wvec2 max;
  bool first = true;
  for (const y::wvec2& v : vertices) {
    min = first ? v : y::min(min, v);
    max = first ? v : y::max(max, v);
    first = false;
  }
  return y::make_pair(min, max);
}

Body::bounds Body::get_full_rotation_bounds(
    const y::wvec2& origin, y::world rotation, const y::wvec2& offset) const
{
  // Where the body will rotate to depends on both its current rotation and
  // the offset of rotation. We rotate it in local coordinates first, then
  // check the bounds in offset coordinates, and finally return in world
  // coordinates.
  y::vector<y::wvec2> vertices;
  get_vertices(vertices, -offset, rotation);

  y::wvec2 min;
  y::wvec2 max;
  bool first = true;
  for (const y::wvec2& v : vertices) {
    min = first ? v : y::min(min, v);
    max = first ? v : y::max(max, v);
    first = false;
  }

  y::world size = y::max(y::abs(min), y::abs(max)).length();
  return y::make_pair(origin + offset + y::wvec2{-size, -size},
                      origin + offset + y::wvec2{size, size});
}

void Body::get_vertices(y::vector<y::wvec2>& output,
                        const y::wvec2& origin, y::world rotation) const
{
  y::wvec2 dr = offset + size / 2;
  y::wvec2 ur = offset + y::wvec2{size[xx], -size[yy]} / 2;
  y::wvec2 dl = offset + y::wvec2{-size[xx], size[yy]} / 2;
  y::wvec2 ul = offset - size / 2;

  // Ensure accuracy for no-rotate objects.
  if (rotation == 0) {
    output.emplace_back(origin + ul);
    output.emplace_back(origin + ur);
    output.emplace_back(origin + dr);
    output.emplace_back(origin + dl);
  }
  else {
    y::wvec2 row_0(cos(rotation), -sin(rotation));
    y::wvec2 row_1(sin(rotation), cos(rotation));

    output.emplace_back(origin + y::wvec2{ul.dot(row_0), ul.dot(row_1)});
    output.emplace_back(origin + y::wvec2{ur.dot(row_0), ur.dot(row_1)});
    output.emplace_back(origin + y::wvec2{dr.dot(row_0), dr.dot(row_1)});
    output.emplace_back(origin + y::wvec2{dl.dot(row_0), dl.dot(row_1)});
  }
}

Collision::Collision(const WorldWindow& world)
  : _world(world)
  , _spatial_hash(256)
{
}

void Collision::render(
    RenderUtil& util,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  y::vector<RenderUtil::line> red;
  y::vector<RenderUtil::line> green;
  y::vector<RenderUtil::line> blue;

  y::size i = 0;
  for (const OrderedBucket& bucket : _world.get_geometry().buckets) {
    for (const Geometry& g : bucket) {
      y::vector<RenderUtil::line>& v = ++i % 2 ? red : green;

      const y::ivec2 max = y::max(g.start, g.end);
      const y::ivec2 min = y::min(g.start, g.end);
      if (y::wvec2(max) >= camera_min && y::wvec2(min) < camera_max) {
        v.emplace_back(RenderUtil::line{y::fvec2(g.start), y::fvec2(g.end)});
      }
    }
  }

  source_list sources;
  get_sources(sources);
  for (const Script* s : sources) {
    y::vector<y::wvec2> vertices;

    for (const entry& b : get_list(*s)) {
      if (!b->collide_mask) {
        continue;
      }

      auto bounds = b->get_bounds(s->get_origin(), s->get_rotation());
      if (!(bounds.second >= camera_min && bounds.first < camera_max)) {
        continue;
      }

      vertices.clear();
      b->get_vertices(vertices, s->get_origin(), s->get_rotation());
      for (y::size i = 0; i < vertices.size(); ++i) {
        blue.emplace_back(RenderUtil::line{
            y::fvec2(vertices[i]),
            y::fvec2(vertices[(i + 1) % vertices.size()])});
      }
    }
  }

  util.render_lines(red, {1.f, 0.f, 0.f, .5f});
  util.render_lines(green, {0.f, 1.f, 0.f, .5f});
  util.render_lines(blue, {0.f, 0.f, 1.f, .5f});
}

y::wvec2 Collision::collider_move(Script& source, const y::wvec2& move) const
{
  const entry_list& bodies = get_list(source);
  if (bodies.empty() || move == y::wvec2()) {
    source.set_origin(source.get_origin() + move);
    return move;
  }

  // TODO: to avoid order-dependent edge-cases (e.g. player standing on platform
  // moving downwards), need to store per-frame list of things which tried to
  // move but were blocked by bodies. If the blocker moves away, try again to
  // move all the things which were blocked by it.
  const OrderedGeometry& geometry = _world.get_geometry();

  // Bounding boxes of the source Bodies.
  auto bounds = get_bounds(bodies, COLLIDE_RESV_WORLD,
                           source.get_origin(), source.get_rotation());
  y::wvec2 min_bound = y::min(bounds.first, move + bounds.first);
  y::wvec2 max_bound = y::max(bounds.second, move + bounds.second);

  y::world min_ratio = 1;
  y::vector<y::wvec2> vertices;
  y::vector<world_geometry> geometries;

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

  // Now do the exact same for bodies.
  bounds = get_bounds(bodies, 0,
                      source.get_origin(), source.get_rotation());
  min_bound = y::min(bounds.first, move + bounds.first);
  max_bound = y::max(bounds.second, move + bounds.second);

  y::vector<Body*> blocking_bodies;
  _spatial_hash.search(blocking_bodies, min_bound, max_bound);

  y::vector<y::wvec2> block_vertices;
  y::vector<world_geometry> block_geometries;
  for (const auto& pointer : bodies) {
    vertices.clear();
    geometries.clear();

    const Body& b = *pointer;
    b.get_vertices(vertices,
                   source.get_origin(), source.get_rotation());
    get_geometries(geometries, vertices);

    for (Body* block : blocking_bodies) {
      if (&block->source == &source ||
          !(b.collide_mask & block->collide_mask)) {
        continue;
      }

      block_vertices.clear();
      block_geometries.clear();

      block->get_vertices(block_vertices,
                          block->source.get_origin(),
                          block->source.get_rotation());
      get_geometries(block_geometries, block_vertices);

      min_ratio = y::min(min_ratio, get_projection_ratio(
                      block_geometries, vertices, move));
      min_ratio = y::min(min_ratio, get_projection_ratio(
                      geometries, block_vertices, -move));
    }
  } 

  // Limit the movement to the minimum blocking ratio (i.e., least 0 <= t <= 1
  // such that moving by t * move is blocked), and recurse in any free unblocked
  // direction.
  const y::wvec2 limited_move = min_ratio * move;
  source.set_origin(limited_move + source.get_origin());
  // TODO: (optionally) recurse?
  return limited_move;
}

y::world Collision::collider_rotate(Script& source, y::world rotate,
                                    const y::wvec2& origin_offset) const
{
  struct local {
    static y::wvec2 origin_displace(const y::wvec2& origin_offset,
                                    y::world rotation)
    {
      y::wvec2 row_0(cos(rotation), -sin(rotation));
      y::wvec2 row_1(sin(rotation), cos(rotation));

      return origin_offset -
          y::wvec2{origin_offset.dot(row_0), origin_offset.dot(row_1)};
    }
  };

  const entry_list& bodies = get_list(source);
  if (bodies.empty() || rotate == 0.) {
    source.set_rotation(y::angle(rotate + source.get_rotation()));
    source.set_origin(source.get_origin() +
                      local::origin_displace(origin_offset, rotate));
    return rotate;
  }

  const OrderedGeometry& geometry = _world.get_geometry();
  y::wvec2 origin = source.get_origin() + origin_offset;

  // Bounding boxes of the source Bodies.
  auto bounds = get_full_rotation_bounds(
      bodies, COLLIDE_RESV_WORLD,
      source.get_origin(), source.get_rotation(), origin_offset);
  y::wvec2 min_bound = bounds.first;
  y::wvec2 max_bound = bounds.second;

  y::world limiting_rotation = y::abs(rotate);
  y::vector<y::wvec2> vertices;
  y::vector<world_geometry> geometries;
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
      // We can't skip geometry defined in opposite direction in the outer loop,
      // since direction depends on body involved. That happens in
      // get_arc_projection.
      if (!(g_min < max_bound && g_max > min_bound)) {
        continue;
      }

      // Very similar strategy to collider_move, except we are projecting points
      // along arcs of circles defined by the distance to the Script's origin
      // (plus offset).
      // Again, we project the first set of points forwards and the second set
      // of points backwards along the arcs.
      world_geometry wg{y::wvec2(g.start), y::wvec2(g.end)};

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
                                wg, vertices, origin, rotate));
        limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                                geometries, {wg.start, wg.end},
                                origin, -rotate));
      }
    }
  }

  bounds = get_full_rotation_bounds(
      bodies, 0, source.get_origin(), source.get_rotation(), origin_offset);
  min_bound = bounds.first;
  max_bound = bounds.second;

  y::vector<Body*> blocking_bodies;
  _spatial_hash.search(blocking_bodies, min_bound, max_bound);

  y::vector<y::wvec2> block_vertices;
  y::vector<world_geometry> block_geometries;
  for (const auto& pointer : bodies) {
    vertices.clear();
    geometries.clear();

    const Body& b = *pointer;
    b.get_vertices(vertices,
                   source.get_origin(), source.get_rotation());
    get_geometries(geometries, vertices);

    for (Body* block : blocking_bodies) {
      if (&block->source == &source ||
          !(b.collide_mask & block->collide_mask)) {
        continue;
      }

      block_vertices.clear();
      block_geometries.clear();

      block->get_vertices(block_vertices,
                          block->source.get_origin(),
                          block->source.get_rotation());
      get_geometries(block_geometries, block_vertices);

      limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                              block_geometries, vertices, origin, rotate));
      limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                              geometries, block_vertices, origin, -rotate));
    }
  } 

  // Rotate.
  const y::world limited_rotation = limiting_rotation * (rotate > 0 ? 1 : -1);
  source.set_rotation(
      y::angle(limited_rotation + source.get_rotation()));
  source.set_origin(
      source.get_origin() +
      local::origin_displace(origin_offset, limited_rotation));
  return limited_rotation;
}

bool Collision::body_check(const Script& source, const Body& body,
                           y::int32 collide_mask) const
{
  const OrderedGeometry& geometry = _world.get_geometry();
  auto bounds = body.get_bounds(source.get_origin(), source.get_rotation());
  y::wvec2 min_bound = bounds.first;
  y::wvec2 max_bound = bounds.second;

  y::vector<y::wvec2> vertices;
  y::vector<world_geometry> geometries;

  body.get_vertices(vertices,
                    source.get_origin(), source.get_rotation());
  get_geometries(geometries, vertices);

  // See collider_move for details.
  for (y::size i = 0; collide_mask & COLLIDE_RESV_WORLD &&
                      i < geometry.buckets.size(); ++i) {
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

  bounds = body.get_bounds(source.get_origin(), source.get_rotation());
  min_bound = bounds.first;
  max_bound = bounds.second;

  y::vector<Body*> blocking_bodies;
  _spatial_hash.search(blocking_bodies, min_bound, max_bound);

  y::vector<y::wvec2> block_vertices;
  y::vector<world_geometry> block_geometries;
  for (Body* block : blocking_bodies) {
    if (&block->source == &body.source ||
        !(collide_mask & block->collide_mask)) {
      continue;
    }

    block_vertices.clear();
    block_geometries.clear();

    block->get_vertices(block_vertices,
                        block->source.get_origin(),
                        block->source.get_rotation());
    get_geometries(block_geometries, block_vertices);

    if (has_intersection(geometries, block_geometries)) {
      return true;
    }
  }

  return false;
}

void Collision::update_spatial_hash(const Script& source)
{ 
  for (const entry& body : get_list(source)) {
    auto bounds = body->get_bounds(source.get_origin(), source.get_rotation());
    _spatial_hash.update(body.get(), bounds.first, bounds.second);
  }
}

void Collision::clear_spatial_hash()
{
  _spatial_hash.clear();
}

void Collision::on_create(const Script& source, Body* obj)
{
  auto bounds = obj->get_bounds(source.get_origin(), source.get_rotation());
  _spatial_hash.update(obj, bounds.first, bounds.second);
}

void Collision::on_destroy(const Script& source, Body* obj)
{
  (void)source;
  _spatial_hash.remove(obj);
}

y::world Collision::get_projection_ratio(
    const y::vector<world_geometry>& geometry,
    const y::vector<y::wvec2>& vertices, const y::wvec2& move) const
{
  y::world min_ratio = 2;
  for (const world_geometry& g : geometry) {
    for (const y::wvec2& v : vertices) {
      min_ratio = y::min(min_ratio, get_projection_ratio(g, v, move, true));
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
    min_ratio = y::min(min_ratio,
                       get_projection_ratio(geometry, v, move, true));
  }
  return min_ratio;
}

y::world Collision::get_projection_ratio(
    const world_geometry& geometry,
    const y::wvec2& vertex, const y::wvec2& move, bool tolerance) const
{
  static const y::world tolerance_factor = 1.0 / 4096;
  world_geometry v{vertex, move + vertex};
  const world_geometry& g = geometry;

  // Skip geometry in the wrong direction.
  if (tolerance) {
    y::wvec2 g_vec = g.end - g.start;
    y::wvec2 normal{g_vec[yy], -g_vec[xx]};
    if (normal.dot(-move) <= 0) {
      return 2;
    }
  }

  // Equations of lines (for t, u in [0, 1]):
  // v(t) = v.start + t * (v.end - v.start)
  // g(u) = g.start + u * (g.end - g.start)
  // Finds t, u such that v(t) = g(u).
  y::world denominator = (g.end - g.start).cross(v.end - v.start);

  // Lines are parallel or coincident.
  if (!denominator) {
    return 2;
  }

  y::world t = (g.end - g.start).cross(v.start - g.start) / -denominator;
  y::world u = (v.end - v.start).cross(g.start - v.start) / denominator;

  // Lines intersect outside of the segments. A small amount of tolerance is
  // necessary when bodies are rotated due to trigonometric innaccuracy.
  // (But we don't want tolerance when this is just a generic line check rather
  // than an actual projection.)
  // TODO: work out what makes us get stuck inside inside corners of zero
  // width.
  if (t < (tolerance ? -tolerance_factor : 0) || t > 1 || u < 0 || u > 1) {
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
                                 const y::vector<world_geometry>& b) const
{
  for (const world_geometry& g: b) {
    if (has_intersection(a, g)) {
      return true;
    }
  }
  return false;
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
  return get_projection_ratio(a, b.start, b.end - b.start, false) <= 1;
}

// TODO: this is supposed to be an optimisation, but actually changes things.
// Essentially, two lines moving flush past each other may get stuck if this
// step doesn't exclude them. May be possible to fix this by chopping off the
// very ends of edge in get_geometries().
Body::bounds Collision::get_bounds(
    const entry_list& bodies, y::int32 collide_mask,
    const y::wvec2& origin, y::world rotation) const
{
  y::wvec2 min;
  y::wvec2 max;
  bool first = true;
  for (const entry& e : bodies) {
    if (!e->collide_mask || (collide_mask &&
                             !(e->collide_mask & collide_mask))) {
      continue;
    }
    auto bounds = e->get_bounds(origin, rotation);
    min = first ? bounds.first : y::min(min, bounds.first);
    max = first ? bounds.second : y::max(max, bounds.second);
    first = false;
  }
  return y::make_pair(min, max);
}

Body::bounds Collision::get_full_rotation_bounds(
    const entry_list& bodies, y::int32 collide_mask,
    const y::wvec2& origin, y::world rotation, const y::wvec2& offset) const
{
  y::wvec2 min;
  y::wvec2 max;
  bool first = true;
  for (const entry& e : bodies) {
    if (!e->collide_mask || (collide_mask &&
                             !(e->collide_mask & collide_mask))) {
      continue;
    }
    auto bounds = e->get_full_rotation_bounds(origin, rotation, offset);
    min = first ? bounds.first : y::min(min, bounds.first);
    max = first ? bounds.second : y::max(max, bounds.second);
    first = false;
  }
  return y::make_pair(min, max);
}

void Collision::get_geometries(y::vector<world_geometry>& output,
                               const y::vector<y::wvec2>& vertices) const
{
  for (y::size i = 0; i < vertices.size(); ++i) {
    output.push_back({vertices[i], vertices[(i + 1) % vertices.size()]});
  }
}
