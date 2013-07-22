#ifndef LIGHTING_H
#define LIGHTING_H

#include "common.h"
#include "gl_handle.h"
#include "lua.h"
#include "vector.h"

struct Geometry;
struct OrderedGeometry;
class GlUtil;
class RenderUtil;
class WorldWindow;

// Point lights and cone lights only, so far!
// TODO: plane lights.
struct Light : y::no_copy {
  Light();

  y::world get_max_range() const;
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

  // Half-angle between 0 and pi through which a conical light shines. Default
  // (pi) gives a regular point light.
  y::world aperture;
  // Angle in which a conical light is pointing.
  y::world angle;
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

  void render_internal(
      RenderUtil& util, const GlFramebuffer& normalbuffer,
      const y::wvec2& camera_min, const y::wvec2& camera_max,
      bool specular) const;

  // Stores trace results. Trace is relative to origin.
  struct trace_key {
    y::wvec2 origin;
    y::world max_range;

    bool operator==(const trace_key& key) const;
    bool operator!=(const trace_key& key) const;
  };
  struct trace_key_hash {
    y::size operator()(const trace_key& key) const;
  };
  typedef y::vector<y::wvec2> light_trace;
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
  typedef y::vector<world_geometry> geometry_entry;
  typedef y::map<y::wvec2, geometry_entry> geometry_map;

  void get_relevant_geometry(y::vector<y::wvec2>& vertex_output,
                             geometry_entry& geometry_output,
                             geometry_map& map_output,
                             const y::wvec2& origin,
                             y::world max_range,
                             const OrderedGeometry& all_geometry) const;
  y::wvec2 get_point_on_geometry(
      const y::wvec2& v, const world_geometry& geometry) const;

  void trace_light_geometry(light_trace& output, y::world max_range,
                            const y::vector<y::wvec2>& vertex_buffer,
                            const geometry_entry& geometry_buffer,
                            const geometry_map& map) const;
  void make_cone_trace(light_trace& output, const light_trace& trace,
                       y::world aperture, y::world angle) const;

  static bool line_intersects_rect(const y::wvec2& start, const y::wvec2& end,
                                   const y::wvec2& min, const y::wvec2& max);

  const WorldWindow& _world;
  GlUtil& _gl;
  GlUnique<GlProgram> _point_light_program;
  GlUnique<GlProgram> _point_light_specular_program;

  GlUnique<GlBuffer<float, 2>> _tri_buffer;
  GlUnique<GlBuffer<float, 2>> _origin_buffer;
  GlUnique<GlBuffer<float, 2>> _range_buffer;
  GlUnique<GlBuffer<float, 4>> _colour_buffer;
  GlUnique<GlBuffer<float, 1>> _layering_buffer;
  GlUnique<GlBuffer<GLushort, 1>> _element_buffer;

  trace_results _trace_results;

};

#endif
