#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "common.h"
#include "gl_handle.h"

class GlUtil;
class RenderUtil;

class Environment : public y::no_copy {
public:

  Environment(GlUtil& util);

  void render_fog_colour(RenderUtil& util,
                         const y::wvec2& origin, const y::wvec2& region,
                         const y::wvec2& tex_offset,
                         y::world fog_min, y::world fog_max,
                         y::world frame, const y::fvec4& colour) const;
  void render_fog_normal(RenderUtil& util,
                         const y::wvec2& origin, const y::wvec2& region) const;

  void render_reflect_colour(
      RenderUtil& util,
      const y::wvec2& origin, const y::wvec2& region,
      const y::wvec2& tex_offset,
      y::world frame, const y::fvec4& colour, float reflect_mix,
      float normal_scaling_reflect, float normal_scaling_refract,
      float reflect_fade_start, float reflect_fade_end,
      bool flip_x, bool flip_y, const y::wvec2& flip_axes,
      const GlFramebuffer& source) const;
  void render_reflect_normal(
      RenderUtil&,
      const y::wvec2& origin, const y::wvec2& region,
      const y::wvec2& tex_offset, y::world frame, float normal_scaling) const;

private:

  GlUnique<GlBuffer<float, 2>> make_rect_buffer(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region) const;

  GlUnique<GlProgram> _fog_program;
  GlUnique<GlProgram> _reflect_program;
  GlUnique<GlProgram> _reflect_normal_program;

  GlUnique<GlTexture2D> _f2d_256;
  GlUnique<GlTexture3D> _f3d_128;
  GlUnique<GlTexture3D> _fv23d_64;

};

#endif
