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
  it->second.list.push_back(y::move_unique(body));
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
      _map.erase(it);
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
      y::fvec4 c = ++i % 2 ? y::fvec4(1.f, 0.f, 0.f, .5f) :
                             y::fvec4(0.f, 1.f, 0.f, .5f);
      const y::ivec2 origin = y::min(g.start, g.end);
      const y::ivec2 size = y::max(y::abs(g.end - g.start), y::ivec2{1, 1});
      if (y::wvec2(origin + size) > camera_min &&
          y::wvec2(origin) < camera_max) {
        util.render_fill(origin, size, c);
      }
    }
  }
}

void Collision::collider_move(Script& source, const y::wvec2& move) const
{
  auto it = _map.find(&source);
  if (it == _map.end() || it->second.list.empty()) {
    source.set_origin(source.get_origin() + move);
    return;
  }

  // So far, we only collide Bodies with world geometry.
  const entry& bodies = it->second;
  const OrderedGeometry& geometry = _world.get_geometry();
  y::world min_bound = y::min(bodies.get_min(), move[xx] + bodies.get_min());
  y::world max_bound = y::max(bodies.get_max(), move[xx] + bodies.get_max());

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
    }
  }
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
