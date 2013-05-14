#include "collision.h"
#include "lua.h"
#include "render_util.h"
#include "world.h"

Body::Body(const Script& source)
  : source(source)
{
}

Collision::Collision(const WorldWindow& world)
  : _world(world)
{
}

Body* Collision::create_body(const Script& source)
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
  auto it = _map.find(&source);
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
  auto it = _map.find(&source);
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
  for (const Geometry& g : _world.get_geometry()) {
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
