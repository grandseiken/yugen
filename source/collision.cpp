#include "collision.h"
#include "render_util.h"
#include "world.h"

// Internal collision functions.
namespace {

  struct world_geometry {
    y::wvec2 start;
    y::wvec2 end;
  };
  typedef CollisionData::entry_list entry_list;
  typedef CollisionData::entry entry;

  // Returns true if the line intersects the circle on more than a point.
  // Finds t for intersection points of the form start + t * (end - start).
  // Treats line as infinite; must check t in [0, 1] if the line is a segment.
  bool line_intersects_circle(
      const y::wvec2& start, const y::wvec2& end,
      const y::wvec2& origin, y::world radius_sq,
      y::world& t_0, y::world& t_1)
  {
    // Equation of line (for t in [0, 1]):
    // f(t) = start + t * (end - start)
    // Equation of circle:
    // |x - origin| = radius
    // Finds all t such that |g(t) - origin| = radius.
    // These are the roots of the equation a * t^2 + b * t + c = 0.
    y::wvec2 f_vec = (end - start);

    y::world a = f_vec.length_squared();
    y::world b = 2 * (start.dot(end) + start.dot(origin) -
                      end.dot(origin) - start.length_squared());
    y::world c = (start - origin).length_squared() - radius_sq;

    y::world sqrtand = b * b - 4 * a * c;

    // If no roots the line and circle do not meet. If one root they touch at a
    // single point, so also cannot be blocking. If a is zero the line is a
    // point.
    if (sqrtand <= 0 || a == 0) {
      return false;
    }

    y::world sqrted = sqrt(sqrtand);
    t_0 = (-b + sqrted) / (2 * a);
    t_1 = (-b - sqrted) / (2 * a);
    return true;
  }

  void get_geometries(y::vector<world_geometry>& output,
                      const y::vector<y::wvec2>& vertices)
  {
    for (y::size i = 0; i < vertices.size(); ++i) {
      output.push_back({vertices[i], vertices[(1 + i) % vertices.size()]});
    }
  }

  y::world get_projection_ratio(
      const world_geometry& geometry,
      const y::wvec2& vertex, const y::wvec2& move, bool tolerance)
  {
    static const y::world tolerance_factor = 1.0 / 1024;
    world_geometry v{vertex, move + vertex};
    const world_geometry& g = geometry;

    // Skip geometry in the wrong direction. I think this is probably duplicated
    // logic now that we have the get_vertices_and_geometries_for_move function.
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
    // (But we don't want tolerance when this is just a generic line check
    // rather than an actual projection.)
    if (t < (tolerance ? -tolerance_factor : 0) || t > 1 || u < 0 || u > 1) {
      return 2;
    }
    return t;
  }

  y::world get_projection_ratio(
      const world_geometry& geometry,
      const y::vector<y::wvec2>& vertices, const y::wvec2& move)
  {
    y::world min_ratio = 2;
    for (const y::wvec2& v : vertices) {
      min_ratio = y::min(min_ratio,
                         get_projection_ratio(geometry, v, move, true));
    }
    return min_ratio;
  }

  y::world get_projection_ratio(
      const y::vector<world_geometry>& geometry,
      const y::vector<y::wvec2>& vertices, const y::wvec2& move)
  {
    y::world min_ratio = 2;
    for (const world_geometry& g : geometry) {
      for (const y::wvec2& v : vertices) {
        min_ratio = y::min(min_ratio, get_projection_ratio(g, v, move, true));
      }
    }
    return min_ratio;
  }

  bool has_intersection(const world_geometry& a,
                        const world_geometry& b)
  {
    return get_projection_ratio(a, b.start, b.end - b.start, false) <= 1;
  }

  bool has_intersection(const y::vector<world_geometry>& a,
                        const world_geometry& b)
  {
    for (const world_geometry& g : a) {
      if (has_intersection(g, b)) {
        return true;
      }
    }
    return false;
  }

  bool has_intersection(const y::vector<world_geometry>& a,
                        const y::vector<world_geometry>& b)
  {
    for (const world_geometry& g: b) {
      if (has_intersection(a, g)) {
        return true;
      }
    }
    return false;
  }

  y::world get_arc_projection(
      const world_geometry& geometry,
      const y::wvec2& vertex,
      const y::wvec2& origin, y::world rotation)
  {
    const world_geometry& g = geometry;
    y::world t_0;
    y::world t_1;
    if (!line_intersects_circle(g.start, g.end, origin,
                                (vertex - origin).length_squared(), t_0, t_1)) {
      return y::abs(rotation);
    }

    // If any root t satisfies 0 <= t <= 1 then it is a block of the circle by
    // the line, so find the angle and limit.
    struct local {
      static y::world limit(y::world t, y::world rotation,
                            y::world initial_angle,
                            const y::wvec2& g_start, const y::wvec2& g_vec,
                            const y::wvec2& origin)
      {
        static const y::world tolerance = 1.0 / 1024;
        // Impact outside the line segment.
        if (t < 0 || t > 1) {
          return y::abs(rotation);
        }
        y::wvec2 impact_rel = g_start + t * g_vec - origin;
        // Skip if the collision is opposite the direction the line is defined
        // in. This is done by forming a line from the origin in the direction
        // of the collision line's normal, and checking which side of it the
        // impact point is on. Similarly, this may now be duplicated logic as we
        // have get_vertices_and_geometries_for_rotate.
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
            ((rotation > 0) != (limiting_angle > initial_angle) ? 2 * y::pi :
                                                                  0);
        // Because of trigonometric inaccuracies and the fact that if we are a
        // tiny bit out in the wrong direction the rotation will not be blocked
        // at all, it's best to have a small amount of tolerance. The tolerance
        // must depend on the distance from the center of rotation, as the
        // distances get bigger.
        if (2 * y::pi - limiting_rotation < tolerance / impact_rel.length()) {
          return limiting_rotation - 2 * y::pi;
        }
        return limiting_rotation;
      }
    };
    y::wvec2 g_vec = g.end - g.start;
    y::world initial_angle = (vertex - origin).angle();
    y::world limiting_rotation = y::min(
        local::limit(t_0, rotation, initial_angle, g.start, g_vec, origin),
        local::limit(t_1, rotation, initial_angle, g.start, g_vec, origin));
    return limiting_rotation;
  }

  y::world get_arc_projection(
      const world_geometry& geometry,
      const y::vector<y::wvec2>& vertices,
      const y::wvec2& origin, y::world rotation)
  {
    y::world limiting_rotation = y::abs(rotation);
    for (const y::wvec2& v : vertices) {
      limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                              geometry, v, origin, rotation));
    }
    return limiting_rotation;
  }

  y::world get_arc_projection(
      const y::vector<world_geometry>& geometry,
      const y::vector<y::wvec2>& vertices,
      const y::wvec2& origin, y::world rotation)
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

  // This filtering looks like an optimisation, but actually affects whether
  // collision will happen 'on a point'. Not a big deal.
  Body::bounds get_bounds(
      const entry_list& bodies, y::int32 collide_mask,
      const y::wvec2& origin, y::world rotation)
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

  Body::bounds get_full_rotation_bounds(
      const entry_list& bodies, y::int32 collide_mask,
      const y::wvec2& origin, y::world rotation, const y::wvec2& offset)
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

  // Get only the vertices and geometries oriented in the direction of a move.
  void get_vertices_and_geometries_for_move(
      y::vector<world_geometry>& geometry_output,
      y::vector<y::wvec2>& vertex_output,
      const y::wvec2& move, const y::vector<y::wvec2>& vertices)
  {
    bool first_keep = false;
    bool keep = false;

    for (y::size i = 0; i < vertices.size(); ++i) {
      const y::wvec2& a = vertices[i];
      const y::wvec2& b = vertices[(1 + i) % vertices.size()];

      bool last_keep = keep;
      keep = move.cross(b - a) > 0;
      if (!i) {
        first_keep = keep;
      }

      if (keep) {
        geometry_output.push_back({a, b});
      }
      if (i && (keep || last_keep)) {
        vertex_output.push_back(vertices[i]);
      }
    }
    if (!vertices.empty() && (keep || first_keep)) {
      vertex_output.push_back(vertices[0]);
    }
  }

  // Similar for rotation.
  void get_vertices_and_geometries_for_rotate(
      y::vector<world_geometry>& geometry_output,
      y::vector<y::wvec2>& vertex_output,
      y::world rotate, const y::wvec2& origin,
      const y::vector<y::wvec2>& vertices)
  {
    bool first_keep = false;
    bool keep = false;

    for (y::size i = 0; i < vertices.size(); ++i) {
      const y::wvec2& a = vertices[i];
      const y::wvec2& b = vertices[(1 + i) % vertices.size()];

      bool last_keep = keep;
      y::wvec2 v = (rotate < 0 ? b : a) - origin;
      y::wvec2 rotate_normal{-v[yy], v[xx]};
      keep = rotate_normal.cross(rotate < 0 ? a - b : b - a) > 0;
      if (!i) {
        first_keep = keep;
      }

      if (keep) {
        geometry_output.push_back({a, b});
      }
      if (i && (keep || last_keep)) {
        vertex_output.push_back(vertices[i]);
      }
    }
    if (!vertices.empty() && (keep || first_keep)) {
      vertex_output.push_back(vertices[0]);
    }
  }

}

// Must be kept in sync with collide.lua.
enum CollideMaskReserved {
  COLLIDE_RESV_NONE = 0,
  COLLIDE_RESV_WORLD = 1,
  COLLIDE_RESV_0 = 2,
  COLLIDE_RESV_1 = 4,
  COLLIDE_RESV_2 = 8,
};

Constraint::Constraint(
    Script& source, Script& target,
    const y::wvec2& source_offset, const y::wvec2& target_offset,
    y::world distance, y::int32 tag)
  : invalidated(false)
  , source(source)
  , target(target)
  , source_offset(source_offset)
  , target_offset(target_offset)
  , distance(distance)
  , tag(tag)
{
}

bool Constraint::is_valid() const
{
  return source.is_valid() && target.is_valid() &&
      !invalidated && source.get() != target.get();
}

Body::Body(Script& source)
  : source(source)
  , collide_type(COLLIDE_RESV_NONE)
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
    output.emplace_back(origin + ul.rotate(rotation));
    output.emplace_back(origin + ur.rotate(rotation));
    output.emplace_back(origin + dr.rotate(rotation));
    output.emplace_back(origin + dl.rotate(rotation));
  }
}

CollisionData::CollisionData()
  : _spatial_hash(256)
{
}

void CollisionData::create_constraint(
    Script& source, Script& target,
    const y::wvec2& source_origin, const y::wvec2& target_origin,
    y::world distance, y::int32 tag)
{
  // Transform points from world coordinates into reference frames of the
  // Scripts.
  y::wvec2 source_offset =
      (source_origin - source.get_origin()).rotate(-source.get_rotation());
  y::wvec2 target_offset =
      (target_origin - target.get_origin()).rotate(-target.get_rotation());

  Constraint* constraint = new Constraint(
      source, target, source_offset, target_offset, distance, tag);
  _constraint_list.emplace_back(constraint);
  _constraint_map[&source].emplace(constraint);
  _constraint_map[&target].emplace(constraint);
}

bool CollisionData::has_constraint(const Script& source) const
{
  auto it = _constraint_map.find(&source);
  if (it == _constraint_map.end()) {
    return false;
  }
  for (Constraint* c : it->second) {
    if (c->is_valid()) {
      return true;
    }
  }
  return false;
}

bool CollisionData::has_constraint(const Script& source, y::int32 tag) const
{
  auto it = _constraint_map.find(&source);
  if (it == _constraint_map.end()) {
    return false;
  }
  for (Constraint* c : it->second) {
    if (c->is_valid() && c->tag == tag) {
      return true;
    }
  }
  return false;
}

void CollisionData::get_constraints(
    y::vector<Script*>& output, const Script& source) const
{
  auto it = _constraint_map.find(&source);
  if (it == _constraint_map.end()) {
    return;
  }
  for (Constraint* c : it->second) {
    if (c->is_valid()) {
      output.emplace_back(&source == c->source.get() ?
                          c->target.get() : c->source.get());
    }
  }
}

void CollisionData::get_constraints(
    y::vector<Script*>& output, const Script& source, y::int32 tag) const
{
  auto it = _constraint_map.find(&source);
  if (it == _constraint_map.end()) {
    return;
  }
  for (Constraint* c : it->second) {
    if (c->is_valid() && c->tag == tag) {
      output.emplace_back(&source == c->source.get() ?
                          c->target.get() : c->source.get());
    }
  }
}

void CollisionData::destroy_constraints(const Script& source)
{
  auto it = _constraint_map.find(&source);
  if (it == _constraint_map.end()) {
    return;
  }
  for (Constraint* c : it->second) {
    c->invalidated = true;
  }
}

void CollisionData::destroy_constraints(const Script& source, y::int32 tag)
{
  auto it = _constraint_map.find(&source);
  if (it == _constraint_map.end()) {
    return;
  }
  for (Constraint* c : _constraint_map[&source]) {
    if (c->tag == tag) {
      c->invalidated = true;
    }
  }
}

void CollisionData::clean_up_constraints()
{
  for (auto it = _constraint_list.begin(); it != _constraint_list.end();) {
    if ((*it)->is_valid()) {
      ++it;
      continue;
    }
    Constraint& constraint = **it;
    _constraint_map[constraint.source.get()].erase(&constraint);
    _constraint_map[constraint.target.get()].erase(&constraint);
    it = _constraint_list.erase(it);
  }
}

void CollisionData::get_bodies_in_region(
    result& output, const y::wvec2& origin, const y::wvec2& region,
    y::int32 collide_mask) const
{
  y::wvec2 min = origin - region / 2;
  y::wvec2 max = origin + region / 2;

  for (auto it = _spatial_hash.search(min, max); it; ++it) {
    Body* body = *it;
    if (collide_mask && !(body->collide_type & collide_mask)) {
      continue;
    }
    if (body_in_region(body, origin, region)) {
      output.emplace_back(body);
    }
  }
}

void CollisionData::get_bodies_in_radius(
    result& output, const y::wvec2& origin, y::world radius,
    y::int32 collide_mask) const
{
  y::wvec2 r{radius, radius};

  for (auto it = _spatial_hash.search(origin - r, origin + r); it; ++it) {
    Body* body = *it;
    if (collide_mask && !(body->collide_type & collide_mask)) {
      continue;
    }
    if (body_in_radius(body, origin, radius)) {
      output.emplace_back(body);
    }
  }
}

void CollisionData::get_bodies_in_body(
    result& output, const Body* body, y::int32 collide_mask) const
{
  auto bounds = body->get_bounds(body->source.get_origin(),
                                 body->source.get_rotation());
  y::wvec2 min_bound = bounds.first;
  y::wvec2 max_bound = bounds.second;

  y::vector<y::wvec2> vertices;
  y::vector<world_geometry> geometries;

  body->get_vertices(vertices,
                     body->source.get_origin(), body->source.get_rotation());
  get_geometries(geometries, vertices);

  y::vector<y::wvec2> block_vertices;
  y::vector<world_geometry> block_geometries;
  for (auto it = _spatial_hash.search(min_bound, max_bound); it; ++it) {
    Body* block = *it;
    if (&block->source == &body->source ||
        !(collide_mask & block->collide_type)) {
      continue;
    }

    block_vertices.clear();
    block_geometries.clear();

    block->get_vertices(block_vertices,
                        block->source.get_origin(),
                        block->source.get_rotation());
    get_geometries(block_geometries, block_vertices);

    if (has_intersection(geometries, block_geometries)) {
      output.emplace_back(block);
    }
  }
}

bool CollisionData::source_in_region(
    const Script& source, const y::wvec2& origin, const y::wvec2& region,
    y::int32 source_collide_mask) const
{
  Body::bounds bounds = get_bounds(get_list(source), 0,
                                   source.get_origin(), source.get_rotation());
  if (bounds.first > origin + region / 2 ||
      bounds.second < origin - region / 2) {
    return false;
  }
  for (const entry& body : get_list(source)) {
    if (source_collide_mask && !(body->collide_type & source_collide_mask)) {
      continue;
    }
    if (body_in_region(body.get(), origin, region)) {
      return true;
    }
  }
  return false;
}

bool CollisionData::source_in_radius(
    const Script& source, const y::wvec2& origin, y::world radius,
    y::int32 source_collide_mask) const
{
  y::wvec2 r{radius, radius};
  Body::bounds bounds = get_bounds(get_list(source), 0,
                                   source.get_origin(), source.get_rotation());
  if (bounds.first > origin + r || bounds.second < origin - r) {
    return false;
  }

  for (const entry& body : get_list(source)) {
    if (source_collide_mask && !(body->collide_type & source_collide_mask)) {
      continue;
    }
    if (body_in_radius(body.get(), origin, radius)) {
      return true;
    }
  }
  return false;
}

bool CollisionData::body_in_region(
    const Body* body, const y::wvec2& origin, const y::wvec2& region) const
{
  y::wvec2 min = origin - region / 2;
  y::wvec2 max = origin + region / 2;

  y::vector<y::wvec2> vertices;
  body->get_vertices(vertices, body->source.get_origin(),
                               body->source.get_rotation());
  if (!vertices.empty() &&
      vertices[0] < max && vertices[0] >= min) {
    return true;
  }
  for (y::size i = 0; i < vertices.size(); ++i) {
    const y::wvec2& a = vertices[i];
    const y::wvec2& b = vertices[(1 + i) % vertices.size()];
    if (y::line_intersects_rect(a, b, min, max)) {
      return true;
    }
  }
  return false;
}

bool CollisionData::body_in_radius(
    const Body* body, const y::wvec2& origin, y::world radius) const
{
  y::vector<y::wvec2> vertices;
  body->get_vertices(vertices, body->source.get_origin(),
                               body->source.get_rotation());

  if (!vertices.empty() &&
      (vertices[0] - origin).length_squared() < radius * radius) {
    return true;
  }

  y::world t_0;
  y::world t_1;
  for (y::size i = 0; i < vertices.size(); ++i) {
    const y::wvec2& a = vertices[i];
    const y::wvec2& b = vertices[(1 + i) % vertices.size()];
    if (line_intersects_circle(a, b, origin, radius * radius, t_0, t_1) &&
        ((t_0 > 0 && t_0 < 1) || (t_1 > 0 && t_1 < 1))) {
      return true;
    }
  }
  return false;
}

void CollisionData::update_spatial_hash(const Script& source)
{
  for (const entry& body : get_list(source)) {
    auto bounds = body->get_bounds(source.get_origin(), source.get_rotation());
    _spatial_hash.update(body.get(), bounds.first, bounds.second);
  }
}

void CollisionData::clear_spatial_hash()
{
  _spatial_hash.clear();
}

const CollisionData::spatial_hash& CollisionData::get_spatial_hash() const
{
  return _spatial_hash;
}

void CollisionData::on_create(const Script& source, Body* obj)
{
  auto bounds = obj->get_bounds(source.get_origin(), source.get_rotation());
  _spatial_hash.update(obj, bounds.first, bounds.second);
}

void CollisionData::on_destroy(Body* obj)
{
  _spatial_hash.remove(obj);
}

Collision::Collision(const WorldWindow& world)
  : _world(world)
{
}

const CollisionData& Collision::get_data() const
{
  return _data;
}

CollisionData& Collision::get_data()
{
  return _data;
}

void Collision::render(
    RenderUtil& util,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  y::vector<RenderUtil::line> green;
  y::vector<RenderUtil::line> blue;

  for (auto it = _world.get_geometry().search(camera_min,
                                             camera_max); it; ++it) {
    green.emplace_back(RenderUtil::line{y::fvec2(it->start),
                                        y::fvec2(it->end)});
  }

  y::vector<y::wvec2> vertices;
  for (auto it = _data.get_spatial_hash().search(camera_min,
                                                 camera_max); it; ++it) {
    const Body* b = *it;
    if (!b->collide_type && !b->collide_mask) {
      continue;
    }

    vertices.clear();
    b->get_vertices(vertices, b->source.get_origin(), b->source.get_rotation());
    for (y::size i = 0; i < vertices.size(); ++i) {
      blue.emplace_back(RenderUtil::line{
          y::fvec2(vertices[i]),
          y::fvec2(vertices[(i + 1) % vertices.size()])});
    }
  }

  util.render_lines(green, {0.f, 1.f, 0.f, .5f});
  util.render_lines(blue, {0.f, 0.f, 1.f, .5f});
}

y::wvec2 Collision::collider_move(
    Body*& first_block_output, Script& source, const y::wvec2& move) const
{
  first_block_output = y::null;
  const entry_list& bodies = _data.get_list(source);
  if (bodies.empty() || move == y::wvec2()) {
    source.set_origin(source.get_origin() + move);
    return move;
  }
  // TODO: need some kind of 'pulling' mechanism. If a platform moves with
  // something on top of it, the thing on top should also move. This should be
  // a joint system: attach a joint at the object's feet every frame if it's
  // standing on something. The same system can be used for other joints, like
  // ropes and chains.

  // Bounding boxes of the source Bodies.
  auto bounds = get_bounds(bodies, COLLIDE_RESV_WORLD,
                           source.get_origin(), source.get_rotation());
  y::wvec2 min_bound = y::min(bounds.first, move + bounds.first);
  y::wvec2 max_bound = y::max(bounds.second, move + bounds.second);

  y::world min_ratio = 1;
  y::vector<y::wvec2> vertices_temp;
  y::vector<y::wvec2> vertices;
  y::vector<world_geometry> geometries;

  // By reversing the nesting of these loops we could skip geometry on a
  // per-body basis. Incurs extra overhead though, and since most things will
  // have one body it's unlikely to be worth it.
  for (auto it = _world.get_geometry().search(min_bound, max_bound); it; ++it) {
    world_geometry wg{y::wvec2(it->start), y::wvec2(it->end)};

    // Skip geometry which is defined opposite the direction of movement..
    y::wvec2 g_vec = wg.end - wg.start;
    y::wvec2 normal{g_vec[yy], -g_vec[xx]};
    if (normal.dot(-move) <= 0) {
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
      vertices_temp.clear();
      vertices.clear();
      geometries.clear();

      b.get_vertices(vertices_temp,
                     source.get_origin(), source.get_rotation());
      get_vertices_and_geometries_for_move(geometries, vertices,
                                           move, vertices_temp);

      min_ratio = y::min(min_ratio, get_projection_ratio(wg, vertices, move));
      min_ratio = y::min(min_ratio, get_projection_ratio(
                      geometries, {wg.start, wg.end}, -move));
    }
  }

  // Now do the exact same for bodies.
  bounds = get_bounds(bodies, 0,
                      source.get_origin(), source.get_rotation());
  min_bound = y::min(bounds.first, move + bounds.first);
  max_bound = y::max(bounds.second, move + bounds.second);

  y::vector<Body*> blocking_bodies;
  _data.get_spatial_hash().search(blocking_bodies, min_bound, max_bound);

  y::vector<y::wvec2> block_vertices;
  y::vector<world_geometry> block_geometries;

  for (const auto& pointer : bodies) {
    vertices_temp.clear();
    vertices.clear();
    geometries.clear();

    const Body& b = *pointer;
    b.get_vertices(vertices_temp,
                   source.get_origin(), source.get_rotation());
    get_vertices_and_geometries_for_move(
        geometries, vertices, move, vertices_temp);

    for (Body* block : blocking_bodies) {
      if (&block->source == &source ||
          !(b.collide_mask & block->collide_type)) {
        continue;
      }

      vertices_temp.clear();
      block_vertices.clear();
      block_geometries.clear();

      block->get_vertices(vertices_temp,
                          block->source.get_origin(),
                          block->source.get_rotation());
      get_vertices_and_geometries_for_move(
          block_geometries, block_vertices, -move, vertices_temp);

      // Save the lowest block_ratio body so we can try to push it.
      y::world block_ratio =
          y::min(get_projection_ratio(block_geometries, vertices, move),
                 get_projection_ratio(geometries, block_vertices, -move));
      if (block_ratio < min_ratio) {
        first_block_output = block;
        min_ratio = block_ratio;
      }
    }
  }

  // Limit the movement to the minimum blocking ratio (i.e., least 0 <= t <= 1
  // such that moving by t * move is blocked), and recurse in any free unblocked
  // direction.
  const y::wvec2 limited_move = min_ratio * move;
  source.set_origin(limited_move + source.get_origin());
  return limited_move;
}

y::wvec2 Collision::collider_move(y::vector<Body*>& push_body_output,
                                  y::vector<y::wvec2>& push_amount_output,
                                  Script& source, const y::wvec2& move,
                                  y::int32 push_mask, y::int32 push_max) const
{
  // TODO: (optionally) recurse against the blocked direction, i.e. slide down
  // a wall if we can? Doesn't matter for 'characters' who walk about through
  // very specific moves, for probably does for dumb physics-y objects.
  Body* first_block;
  y::wvec2 limited_move = collider_move(first_block, source, move);

  if (!first_block || !push_mask || push_max <= 0 ||
      !(push_mask & first_block->collide_type)) {
    return limited_move;
  }

  // Push it. I am the pusher robot. We are here to protect you from the
  // terrible secret of space.
  Script& block_source = first_block->source;
  y::wvec2 remaining_move = move - limited_move;

  // Attempt to push the blocker.
  y::vector<Body*> push_bodies;
  y::vector<y::wvec2> push_amounts;
  y::wvec2 block_move = collider_move(push_bodies, push_amounts,
                                      block_source, remaining_move,
                                      push_mask, push_max - 1);
  // If it didn't move, we're stuck, continue as normal.
  if (block_move == y::wvec2()) {
    return limited_move;
  }

  // Store the output data recursively.
  push_body_output.emplace_back(first_block);
  push_amount_output.emplace_back(block_move);

  push_body_output.insert(push_body_output.end(),
                          push_bodies.begin(), push_bodies.end());
  push_amount_output.insert(push_amount_output.end(),
                            push_amounts.begin(), push_amounts.end());
  push_bodies.clear();
  push_amounts.clear();

  // Otherwise, try to move us again. We need to subtract the number
  // of objects we already pushed otherwise ordering can mean we push
  // more than push_max in general.
  y::wvec2 recursive_move = collider_move(
      push_bodies, push_amounts, source, block_move,
      push_mask, push_max - push_body_output.size());

  // If we got stuck closer than the blocked object, move the blockers we
  // already pushed back to where we got stuck.
  if (recursive_move.length_squared() < block_move.length_squared()) {
    for (y::size i = 0; i < push_body_output.size(); ++i) {
      Body* b = push_body_output[i];
      Body* ignore;
      // Need to collider_move them back otherwise odd things can happen in
      // more-than-two-body interactions.
      push_amount_output[i] +=
          collider_move(ignore, b->source, recursive_move - block_move);
    }
  }

  push_body_output.insert(push_body_output.end(),
                          push_bodies.begin(), push_bodies.end());
  push_amount_output.insert(push_amount_output.end(),
                            push_amounts.begin(), push_amounts.end());
  return limited_move + recursive_move;
}

y::world Collision::collider_rotate(Script& source, y::world rotate,
                                    const y::wvec2& origin_offset) const
{
  // TODO: rotation still seems to be a little bit inconsistent as to
  // when a thing touching a surface at a point will rotate. Seems like
  // it always will for the first half of the rotation, but won't for the
  // second half (for the player, at least).
  struct local {
    static y::wvec2 origin_displace(const y::wvec2& origin_offset,
                                    y::world rotation)
    {
      return origin_offset - origin_offset.rotate(rotation);
    }
  };

  const entry_list& bodies = _data.get_list(source);
  if (bodies.empty() || rotate == 0.) {
    source.set_rotation(y::angle(rotate + source.get_rotation()));
    source.set_origin(source.get_origin() +
                      local::origin_displace(origin_offset, rotate));
    return rotate;
  }
  y::wvec2 origin = source.get_origin() + origin_offset;

  // Bounding boxes of the source Bodies.
  auto bounds = get_full_rotation_bounds(
      bodies, COLLIDE_RESV_WORLD,
      source.get_origin(), source.get_rotation(), origin_offset);
  y::wvec2 min_bound = bounds.first;
  y::wvec2 max_bound = bounds.second;

  y::world limiting_rotation = y::abs(rotate);
  y::vector<y::wvec2> vertices_temp;
  y::vector<y::wvec2> vertices;
  y::vector<world_geometry> geometries;

  // See collider_move for details.
  for (auto it = _world.get_geometry().search(min_bound, max_bound); it; ++it) {
    world_geometry wg{y::wvec2(it->start), y::wvec2(it->end)};

    // Check geometry orientation.
    y::wvec2 v = (rotate > 0 ? wg.end : wg.start) - origin;
    y::wvec2 rotate_normal{-v[yy], v[xx]};
    if (!rotate_normal.cross(rotate > 0 ? wg.start - wg.end :
                                          wg.end - wg.start) > 0) {
      continue;
    }

    // Very similar strategy to collider_move, except we are projecting points
    // along arcs of circles defined by the distance to the Script's origin
    // (plus offset).
    // Again, we project the first set of points forwards and the second set
    // of points backwards along the arcs.

    for (const auto& pointer : bodies) {
      const Body& b = *pointer;
      if (!(b.collide_mask & COLLIDE_RESV_WORLD)) {
        continue;
      }
      vertices_temp.clear();
      vertices.clear();
      geometries.clear();

      b.get_vertices(vertices_temp,
                     source.get_origin(), source.get_rotation());
      get_vertices_and_geometries_for_rotate(
          geometries, vertices, rotate, origin, vertices_temp);

      limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                              wg, vertices, origin, rotate));
      limiting_rotation = y::min(limiting_rotation, get_arc_projection(
                              geometries, {wg.start, wg.end},
                              origin, -rotate));
    }
  }

  bounds = get_full_rotation_bounds(
      bodies, 0, source.get_origin(), source.get_rotation(), origin_offset);
  min_bound = bounds.first;
  max_bound = bounds.second;

  y::vector<Body*> blocking_bodies;
  _data.get_spatial_hash().search(blocking_bodies, min_bound, max_bound);

  y::vector<y::wvec2> block_vertices;
  y::vector<world_geometry> block_geometries;
  for (const auto& pointer : bodies) {
    vertices_temp.clear();
    vertices.clear();
    geometries.clear();

    const Body& b = *pointer;
    b.get_vertices(vertices_temp,
                   source.get_origin(), source.get_rotation());
    get_vertices_and_geometries_for_rotate(
        geometries, vertices, rotate, origin, vertices_temp);

    for (Body* block : blocking_bodies) {
      if (&block->source == &source ||
          !(b.collide_mask & block->collide_type)) {
        continue;
      }

      vertices_temp.clear();
      block_vertices.clear();
      block_geometries.clear();

      block->get_vertices(vertices_temp,
                          block->source.get_origin(),
                          block->source.get_rotation());
      get_vertices_and_geometries_for_rotate(
          block_geometries, block_vertices, -rotate, origin, vertices_temp);

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

bool Collision::source_check(
    const Script& source,
    y::int32 source_collide_mask, y::int32 target_collide_mask) const
{
  for (const entry& e : _data.get_list(source)) {
    if (source_collide_mask && !(source_collide_mask && e->collide_type)) {
      continue;
    }
    if (body_check(e.get(), target_collide_mask)) {
      return true;
    }
  }
  return false;
}

bool Collision::body_check(const Body* body, y::int32 collide_mask) const
{
  auto bounds = body->get_bounds(body->source.get_origin(),
                                 body->source.get_rotation());
  y::wvec2 min_bound = bounds.first;
  y::wvec2 max_bound = bounds.second;

  y::vector<y::wvec2> vertices;
  y::vector<world_geometry> geometries;

  body->get_vertices(vertices,
                     body->source.get_origin(), body->source.get_rotation());
  get_geometries(geometries, vertices);

  // See collider_move for details.
  for (auto it = _world.get_geometry().search(min_bound, max_bound);
       it && collide_mask & COLLIDE_RESV_WORLD; ++it) {
    if (has_intersection(geometries, world_geometry{y::wvec2(it->start),
                                                    y::wvec2(it->end)})) {
      return true;
    }
  }

  y::vector<y::wvec2> block_vertices;
  y::vector<world_geometry> block_geometries;
  for (auto it = _data.get_spatial_hash().search(min_bound,
                                                 max_bound); it; ++it) {
    const Body* block = *it;
    if (&block->source == &body->source ||
        !(collide_mask & block->collide_type)) {
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
