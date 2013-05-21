#include "collision.h"
#include "lua.h"
#include "render_util.h"
#include "world.h"

Body::Body(const Script& source)
  : source(source)
{
}

y::world Body::get_min() const
{
  return offset[xx] - size[xx] / 2;
}

y::world Body::get_max() const
{
  return offset[xx] + size[xx] / 2;
}

Collision::Collision(const WorldWindow& world)
  : _world(world)
{
}

Body* Collision::create_body(Script& source)
{
  Body* body = new Body(source);
  auto it = _map.find(&source);
  if (it != _map.end() && !it->second.ref.is_valid()) {
    _map.erase(it);
    it = _map.end();
  }
  if (it == _map.end()) {
    it = _map.insert(y::make_pair(&source, entry{
             ConstScriptReference(source), y::vector<body_entry>()})).first;
  }
  it->second.list.emplace_back(body);
  return body;
}

void Collision::destroy_body(const Script& source, Body* body)
{
  auto it = _map.find(const_cast<Script*>(&source));
  if (it != _map.end()) {
    for (auto jt = it->second.list.begin(); jt != it->second.list.end(); ++jt) {
      if (jt->get() == body) {
        it->second.list.erase(jt);
        break;
      }
    }
    if (it->second.list.empty() || !it->second.ref.is_valid()) {
      _map.erase(it);
    }
  }
}

void Collision::destroy_bodies(const Script& source)
{
  auto it = _map.find(const_cast<Script*>(&source));
  if (it != _map.end()) {
    _map.erase(it);
  }
}

void Collision::clean_up()
{
  for (auto it = _map.begin(); it != _map.end();) {
    if (it->second.ref.is_valid()) {
      ++it;
    }
    else {
      it = _map.erase(it);
    }
  }
}

void Collision::render(
    RenderUtil& util,
    const y::wvec2& camera_min, const y::wvec2& camera_max) const
{
  y::size i = 0;
  for (const OrderedBucket& bucket : _world.get_geometry().buckets) {
    for (const Geometry& g : bucket) {
      const y::fvec4 c = ++i % 2 ? y::fvec4(1.f, 0.f, 0.f, .5f) :
                                   y::fvec4(0.f, 1.f, 0.f, .5f);
      const y::ivec2 size = y::max(y::abs(g.end - g.start), y::ivec2{1, 1});
      y::ivec2 origin = y::min(g.start, g.end);
      if (g.start[xx] > g.end[xx]) {
        origin[yy] -= 1;
      }
      if (g.start[yy] < g.end[yy]) {
        origin[xx] -= 1;
      }
      if (y::wvec2(origin + size) > camera_min &&
          y::wvec2(origin) < camera_max) {
        util.render_fill(origin, size, c);
      }
    }
  }
  const y::fvec4 c{0.f, 0.f, 1.f, .5f};
  const y::wvec2 round{.5f, .5f};
  for (const auto& pair : _map) {
    for (const body_entry& b : pair.second.list) {
      util.render_outline(
          y::ivec2(
              pair.second.ref->get_origin() + b->offset - b->size / 2 + round),
          y::ivec2(b->size + round), c);
    }
  }
}

void Collision::collider_move(Script& source, const y::wvec2& move) const
{
  auto it = _map.find(&source);
  if (it == _map.end() || it->second.list.empty() || move == y::wvec2()) {
    source.set_origin(source.get_origin() + move);
    return;
  }

  // So far, we only collide Bodies with world geometry.
  // TODO: collide bodies with bodies; have some sort of collision masks;
  // allow bodies which only detect overlaps; etc.
  const entry& bodies = it->second;
  const OrderedGeometry& geometry = _world.get_geometry();

  // X-coordinate bounds of the source Bodies.
  y::world min_bound = source.get_origin()[xx] + bodies.get_min();
  min_bound = y::min(min_bound, move[xx] + min_bound);
  y::world max_bound = source.get_origin()[xx] + bodies.get_max();
  max_bound = y::max(max_bound, move[xx] + max_bound);

  y::world min_ratio = 1;
  // Eliminating geometry by bucket and order depends heavily on structure of
  // OrderedGeometry.
  for (y::size i = 0; i < geometry.buckets.size(); ++i) {
    // Skip buckets we're completely outside. If this gets too slow, we can
    // bucket by both x and y.
    y::int32 g_max = geometry.get_max_for_bucket(i);
    if (min_bound >= g_max) {
      continue;
    }

    for (const Geometry& g : geometry.buckets[i]) {
      // By ordering, once we see a high enough minimum we can break.
      y::int32 g_min = y::min(g.start[xx], g.end[xx]);
      if (g_min >= max_bound) {
        break;
      }
      // Skip geometry which is defined opposite the direction of movement.
      y::world normal = (g.end - g.start).angle() - y::pi / 2;
      if (y::angle_distance(normal, move.angle()) < y::pi / 2) {
        continue;
      }
      // TODO: skip geometry for which a general bounding-box check fails.

      // Now: project each relevant (depending on direction of movement) vertex
      // of each body along the movement vector to form a line. Find the
      // minimum blocking ratio among each intersection of the geometry with
      // these, and also any endpoint of geometry contained within the
      // projection.
      bool project_downright =
          move[xx] > 0 || move[yy] > 0;
      bool project_upright =
          move[xx] > 0 || move[yy] < 0;
      bool project_downleft =
          move[xx] < 0 || move[yy] > 0;
      bool project_upleft =
          move[xx] < 0 || move[yy] < 0;

      for (const auto& pointer : bodies.list) {
        const Body& b = *pointer;
        y::wvec2 origin = source.get_origin() + b.offset;
        if (project_downright) {
          min_ratio = y::min(min_ratio, get_move_ratio(
                          g, origin + b.size / 2, move));
        }
        if (project_upright) {
          min_ratio = y::min(min_ratio, get_move_ratio(
                          g, origin + y::wvec2{1.0, -1.0} * b.size / 2, move));
        }
        if (project_downleft) {
          min_ratio = y::min(min_ratio, get_move_ratio(
                          g, origin + y::wvec2{-1.0, 1.0} * b.size / 2, move));
        }
        if (project_upleft) {
          min_ratio = y::min(min_ratio, get_move_ratio(
                          g, origin + y::wvec2{-1.0, -1.0} * b.size / 2, move));
        }
        // TODO: need to do leading edges of polygon as well, not just the
        // projected vertices! (For slopes to work, anyway.) Maybe we should
        // do all edges and projected vertices, for extra stability.
      }
    }
  }
  // Limit the movement to the minimum blocking ratio (i.e., least 0 <= t <= 1
  // such that moving by t * move is blocked, and recurse in any free unblocked
  // direction.
  source.set_origin(min_ratio * move + source.get_origin());
  // TODO: recurse.
}

y::world Collision::get_move_ratio(
    const Geometry& geometry,
    const y::wvec2& vertex, const y::wvec2& move) const
{
  struct world_geometry {
    y::wvec2 start;
    y::wvec2 end;
  };
  world_geometry v{vertex, move + vertex};
  world_geometry g{y::wvec2(geometry.start), y::wvec2(geometry.end)};

  // Equations of lines (for t, u in [0, 1]):
  // v(t) = v.start + (v.end - v.start) * t
  // g(u) = g.start + (g.end - g.start) * u
  // Finds t, u such that v(t) = g(u).
  y::world denominator =
      (g.end[xx] - g.start[xx]) * (v.end[yy] - v.start[yy]) -
      (g.end[yy] - g.start[yy]) * (v.end[xx] - v.start[xx]);

  // Lines are parallel or coincident.
  if (!denominator) {
    return 1;
  }

  y::world t = (
      (g.end[xx] - g.start[xx]) * (v.start[yy] - g.start[yy]) -
      (g.end[yy] - g.start[yy]) * (v.start[xx] - g.start[xx])) / -denominator;
  y::world u = (
      (v.end[xx] - v.start[xx]) * (g.start[yy] - v.start[yy]) -
      (v.end[yy] - v.start[yy]) * (g.start[xx] - v.start[xx])) / denominator;

  // Lines intersect outside of the segments.
  if (t < 0 || t > 1 || u < 0 || u > 1) {
    return 1;
  }
  // TODO: need to also check endpoints of geometry.
  return t;
}

y::world Collision::entry::get_min() const
{
  y::world min = 0;
  bool first = true;
  for (const body_entry& e : list) {
    min = first ? e->get_min() : y::min(min, e->get_min());
    first = false;
  }
  return min;
}

y::world Collision::entry::get_max() const
{
  y::world max = 0;
  bool first = true;
  for (const body_entry& e : list) {
    max = first ? e->get_max() : y::max(max, e->get_max());
    first = false;
  }
  return max;
}
