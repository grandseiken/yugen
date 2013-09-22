#ifndef COLLISION_H
#define COLLISION_H

#include "common.h"
#include "lua.h"
#include "spatial_hash.h"
#include "vector.h"

struct Body;
class RenderUtil;
class WorldWindow;

// A Constraint links two Scripts such that the distance between them cannot
// exceed some amount. For simplicity, constraints to the world itself should
// be implemented using dummy target Scripts.
struct Constraint : y::no_copy {
  Constraint(Script& source, Script& target,
             const y::wvec2& source_offset, const y::wvec2& target_offset,
             bool source_fixed, bool target_fixed,
             y::world distance, y::int32 tag);

  // Set invalidated to true in order to destroy the Constraint.
  bool is_valid() const;
  bool invalidated;

  // Get the other Script if we know one of them.
  const Script& other(const Script& script) const;
  /***/ Script& other(const Script& script);

  // Get the offset or fixing corresponding to one of the Scripts.
  const y::wvec2& offset(const Script& script) const;
  bool fixed(const Script& script) const;
  
  ScriptReference source;
  ScriptReference target;

  // Positions of endpoints relative to source and target reference frames.
  y::wvec2 source_offset;
  y::wvec2 target_offset;

  // Maximum distance, and whether the source and target are fixed (can't be
  // pulled by this Constraint).
  y::world distance;
  bool source_fixed;
  bool target_fixed;

  // The tag is an arbitrary value that can be used for lookup.
  y::int32 tag;
};

// A Body is, thus far, a rectangular area of some size, whose center
// is offset some amount from the origin of the source Script.
struct Body : y::no_copy {
  Body(Script& source);

  // Get bounds (on x-axis). Potentially this could be made virtual and
  // overridden for other shapes.
  typedef y::pair<y::wvec2, y::wvec2> bounds;
  bounds get_bounds(const y::wvec2& origin, y::world rotation) const;
  bounds get_full_rotation_bounds(
      const y::wvec2& origin, y::world rotation, const y::wvec2& offset) const;

  void get_vertices(y::vector<y::wvec2>& output,
                    const y::wvec2& origin, y::world rotation) const;

  Script& source;
  y::wvec2 offset;
  y::wvec2 size;

  // Bodies are blocked by any bodies whose collide_type matches their
  // collide_mask. Collide_mask only affects what the body will be blocked
  // by when moving or rotating; lookup and filtering functions are based
  // on the collide_type.
  y::int32 collide_type;
  y::int32 collide_mask;
};

// Data structures for storing Constraints.
class ConstraintData {
public:

  // Creates a Constraint between two bodies. The source and target parameters
  // are given in world coordinates and transformed automatically.
  void create_constraint(
      Script& source, Script& target,
      const y::wvec2& source_origin, const y::wvec2& target_origin,
      bool source_fixed, bool target_fixed,
      y::world distance, y::int32 tag);

  typedef y::set<Constraint*> constraint_set;
  const constraint_set& get_constraint_set(const Script& source) const;

  // Check if a Script has any constraints, optionally having the given tag.
  bool has_constraint(const Script& source) const;
  bool has_constraint(const Script& source, y::int32 tag) const;

  // Returns the other Scripts involved in any constraints, optionally having
  // the given tag.
  void get_constraints(y::vector<Script*>& output, const Script& source) const;
  void get_constraints(y::vector<Script*>& output, const Script& source,
                       y::int32 tag) const;

  // Destroys all constraints, optionally having the given tag.
  void destroy_constraints(const Script& source);
  void destroy_constraints(const Script& source, y::int32 tag);

  // Must be called every so often to clean up Constraints.
  void clean_up();

private:

  typedef y::vector<y::unique<Constraint>> constraint_list;
  typedef y::map<const Script*, constraint_set> constraint_map;
  constraint_list _constraint_list;
  constraint_map _constraint_map;
  constraint_set _no_constraints;

};

class CollisionData : public ScriptMap<Body> {
public:

  CollisionData();
  ~CollisionData() override {}

  typedef y::vector<Body*> result;

  // Finds all bodies that currently overlap the given region, radius, or body,
  // with an optional mask on thier collide_type.
  void get_bodies_in_region(
      result& output, const y::wvec2& origin, const y::wvec2& region,
      y::int32 collide_mask) const;
  void get_bodies_in_radius(
      result& output, const y::wvec2& origin, y::world radius,
      y::int32 collide_mask) const;
  void get_bodies_in_body(
      result& output, const Body* body, y::int32 collide_mask) const;

  // Returns true iff any of the source's bodies overlap the region or radius.
  bool source_in_region(
      const Script& source, const y::wvec2& origin, const y::wvec2& region,
      y::int32 source_collide_mask) const;
  bool source_in_radius(
      const Script& source, const y::wvec2& origin, y::world radius,
      y::int32 source_collide_mask) const;

  // Returns true iff the given body overlaps the region or radius.
  bool body_in_region(
      const Body* body, const y::wvec2& origin, const y::wvec2& region) const;
  bool body_in_radius(
      const Body* body, const y::wvec2& origin, y::world radius) const;

  // This must be called whenever a Script's position or rotation changes in
  // order to update the bodies in the spatial hash.
  void update_spatial_hash(const Script& source);
  void clear_spatial_hash();

  typedef SpatialHash<Body*, y::world, 2> spatial_hash;
  const spatial_hash& get_spatial_hash() const;

protected:

  virtual void on_create(const Script& source, Body* obj) override;
  virtual void on_destroy(Body* obj) override;

private:

  // Spatial hash for fast lookup.
  spatial_hash _spatial_hash;

};

// Keeps a record of Bodies and handles sweeping and collision.
class Collision {
public:

  Collision(const WorldWindow& world);

  const CollisionData& get_data() const;
  /***/ CollisionData& get_data();

  const ConstraintData& get_constraints() const;
  /***/ ConstraintData& get_constraints();

  void render(RenderUtil& util,
              const y::wvec2& camera_min, const y::wvec2& camera_max) const;

  // Push_mask controls the type of objects that can be pushed out of the way,
  // and push_max the maximum number that can be pushed at once.
  y::wvec2 collider_move(y::vector<Script*>& push_script_output,
                         y::vector<y::wvec2>& push_amount_output,
                         Script& source, const y::wvec2& move,
                         y::int32 push_mask, y::int32 push_max) const;

  // Rotation is currently somewhat of a second-class citizen. Rotations cannot
  // push other Bodies (they will always be blocked), and nor can they affect
  // other Scripts via Constraints.
  y::world collider_rotate(Script& source, y::world rotate,
                           const y::wvec2& origin_offset) const;

  // Check if a body or source overlaps a collide mask.
  bool source_check(
      const Script& source,
      y::int32 source_collide_mask,
      y::int32 target_collide_mask) const;
  bool body_check(
      const Body* body, y::int32 collide_mask) const;

private:

  // Primitive move function. Moves the collider as far as it can go and stops.
  y::world collider_move_raw(Body*& first_blocker_output,
                             Script& source, const y::wvec2& move) const;

  // Move function with pushing.
  y::world collider_move_push(y::vector<Script*>& push_script_output,
                              y::vector<y::wvec2>& push_amount_output,
                              Script& source, const y::wvec2& move,
                              y::int32 push_mask, y::int32 push_max) const;

  // Move function respecting constraints.
  y::world collider_move_constrained(
      y::vector<Script*>& push_script_output,
      y::vector<y::wvec2>& push_amount_output,
      Script& source, const y::wvec2& move,
      y::int32 push_mask, y::int32 push_max,
      const ConstraintData::constraint_set& ignored_constraints) const;

  const WorldWindow& _world;
  CollisionData _data;
  ConstraintData _constraints;

};

#endif
