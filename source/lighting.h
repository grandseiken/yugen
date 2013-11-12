#ifndef LIGHTING_H
#define LIGHTING_H

#include "common.h"
#include "render/gl_handle.h"
#include "lua.h"
#include "world.h"
#include "vector.h"

class GlUtil;
class RenderUtil;

// Data for various kinds of light.
struct Light : y::no_copy {
  Light(const Script& source);

  const Script& source;

  // Displacement from the owning Script's origin. For plane lights, the offset
  // defines the plane from (origin - offset) to (origin + offset).
  y::wvec2 offset;

  // The range at which the light still has full intensity.
  y::world full_range;

  // The additional range through which the intensity falls off to zero.
  y::world falloff_range;

  // Layering value. Similar in some ways to a depth value. Lights will only
  // affect areas where the third component of the normalbuffer, the layerying
  // component, is greater than the layering value of the light.
  y::world layer_value;

  // Red, green, blue, and intensity.
  y::fvec4 colour;

  // Angle in which a conical light is pointing.
  y::world angle;

  // Half-angle between 0 and pi through which a conical light shines. Default
  // (pi) gives a regular point light.
  y::world aperture;

  // Whether this is a plane light. Such a light emits light in a particular
  // direction all along a plane (by which, through abuse of terminology, we
  // really mean a hyperplane; a line). Default (0, 0) gives a regular point
  // light.
  y::wvec2 normal_vec;

  // Handy functions.
  y::world get_max_range() const;
  y::wvec2 get_origin(const y::wvec2& origin) const;
  y::wvec2 get_offset() const;
  bool is_planar() const;
  bool overlaps_rect(const y::wvec2& origin,
                     const y::wvec2& min, const y::wvec2& max) const;
};

// Keeps a record of Lights and handles fancy lighting algorithms.
class Lighting : public ScriptMap<Light> {
public:

  Lighting(const WorldWindow& world, GlUtil& gl);
  ~Lighting() override {};

  // Lighting functions.
  void recalculate_traces(
      const y::wvec2& camera_min, const y::wvec2& camera_max);
  void clear_results_and_cache();

  void render_traces(
      RenderUtil& util,
      const y::wvec2& camera_min, const y::wvec2& camera_max) const;
  void render_lightbuffer(
      RenderUtil& util, const GlFramebuffer& normalbuffer,
      const y::wvec2& camera_min, const y::wvec2& camera_max) const;
  void render_specularbuffer(
      RenderUtil& util, const GlFramebuffer& normalbuffer,
      const y::wvec2& camera_min, const y::wvec2& camera_max) const;

private:

  typedef y::vector<y::wvec2> light_trace;

  // Add all the data for a vertex of a lit shape.
  void add_vertex(const y::wvec2& origin, const y::wvec2& trace,
                  const Light& light) const;
  // Add a triangle by element indices if necessary.
  void add_triangle(
      y::vector<GLushort>& element_data,
      const y::wvec2& min, const y::wvec2& max,
      y::size start_index, y::size a, y::size b, y::size c,
      const y::wvec2& at, const y::wvec2& bt, const y::wvec2& ct) const;

  void render_internal(
      RenderUtil& util, const GlFramebuffer& normalbuffer,
      const y::wvec2& camera_min, const y::wvec2& camera_max,
      bool specular) const;

  void render_angular_internal(
      const light_trace& trace, const Light& light, const y::wvec2& origin,
      const y::wvec2& camera_min, const y::wvec2& camera_max) const;
  void render_planar_internal(
      const light_trace& trace, const Light& light, const y::wvec2& origin,
      const y::wvec2& camera_min, const y::wvec2& camera_max) const;

  // Stores trace results. Trace is relative to origin.
  struct trace_key {
    y::wvec2 origin;
    y::world max_range;

    // Key-fields for plane lights.
    y::wvec2 normal_vec;
    y::wvec2 offset;

    bool operator==(const trace_key& key) const;
    bool operator!=(const trace_key& key) const;
  };
  struct trace_key_hash {
    y::size operator()(const trace_key& key) const;
  };
  typedef y::map<trace_key, light_trace, trace_key_hash> trace_results;

  // Internal lighting functions.
  struct world_geometry {
    world_geometry();
    world_geometry(const Geometry& geometry);
    world_geometry(const y::wvec2& start, const y::wvec2& end);

    bool operator==(const world_geometry& g) const;
    bool operator!=(const world_geometry& g) const;

    y::wvec2 start;
    y::wvec2 end;
  };
  struct world_geometry_hash {
    y::size operator()(const world_geometry& g) const;
  };
  typedef y::vector<world_geometry> geometry_entry;
  typedef y::map<y::wvec2, geometry_entry> geometry_map;
  typedef y::set<world_geometry, world_geometry_hash> geometry_set;

  // Pair of functions for finding all vertices and geometries that might
  // affect the light output in the angular and planar settings.
  static void get_relevant_geometry(
      y::vector<y::wvec2>& vertex_output, geometry_entry& geometry_output,
      geometry_map& map_output, const Light& light, const y::wvec2& origin,
      const WorldGeometry::geometry_hash& all_geometry, bool planar);

  static void get_angular_relevant_geometry(
      y::vector<y::wvec2>& vertex_output, geometry_entry& geometry_output,
      geometry_map& map_output, const Light& light, const y::wvec2& origin,
      const WorldGeometry::geometry_hash& all_geometry);

  static void get_planar_relevant_geometry(
      y::vector<y::wvec2>& vertex_output, geometry_entry& geometry_output,
      geometry_map& map_output, const Light& light, const y::wvec2& origin,
      const WorldGeometry::geometry_hash& all_geometry);

  // Helper functions.
  static y::wvec2 get_angular_point_on_geometry(
      const y::wvec2& v, const world_geometry& geometry);
  static y::wvec2 get_planar_point_on_geometry(
      const y::wvec2& normal_vec, const y::wvec2& v,
      const world_geometry& geometry);

  // Pair of functions for tracing angular and planar light geometry.
  static void trace_light_geometry(light_trace& output, const Light& light,
                                   const y::vector<y::wvec2>& vertex_buffer,
                                   const geometry_entry& geometry_buffer,
                                   const geometry_map& map, bool planar);

  static void trace_angular_light_geometry(
      light_trace& output, const Light& light,
      const y::vector<y::wvec2>& vertex_buffer,
      const geometry_entry& geometry_buffer,
      const geometry_map& map);

  static void trace_planar_light_geometry(
      light_trace& output, const Light& light,
      const y::vector<y::wvec2>& vertex_buffer,
      const geometry_entry& geometry_buffer,
      const geometry_map& map);

  // Converts a point-light trace into a cone-light trace.
  static void make_cone_trace(light_trace& output, const light_trace& trace,
                              y::world angle, y::world aperture);

  const WorldWindow& _world;
  GlUtil& _gl;
  GlUnique<GlProgram> _light_program;
  GlUnique<GlProgram> _light_specular_program;

  GlDatabuffer<float, 2> _tri;
  GlDatabuffer<float, 2> _origin;
  GlDatabuffer<float, 2> _range;
  GlDatabuffer<float, 4> _colour;
  GlDatabuffer<float, 1> _layering;

  mutable y::vector<y::vector<GLushort>> _element_data;
  GlUnique<GlBuffer<GLushort, 1>> _element_buffer;

  trace_results _trace_results;

};

#endif
