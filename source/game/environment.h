#ifndef GAME__ENVIRONMENT_H
#define GAME__ENVIRONMENT_H

#include "../common.h"
#include "../render/gl_handle.h"

class GlUtil;
class RenderUtil;

class Environment : public y::no_copy {
public:

  struct fog_params {
    y::world layering_value;
    y::wvec2 tex_offset;
    y::world frame;

    y::fvec4 colour;
    y::world fog_min;
    y::world fog_max;

    bool normal_only;
  };

  struct reflect_params {
    y::world layering_value;
    y::wvec2 tex_offset;
    y::world frame;

    y::fvec4 colour;
    y::world reflect_mix;
    y::world light_passthrough;

    y::world normal_scaling;
    y::world normal_scaling_reflect;
    y::world normal_scaling_refract;

    y::world reflect_fade_start;
    y::world reflect_fade_end;
    bool flip_x;
    bool flip_y;
    y::wvec2 flip_axes;

    y::world wave_height;
    y::world wave_scale;

    bool normal_only;
  };

  Environment(GlUtil& util, bool fake);

  void render_fog_colour(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
      const fog_params& params) const;
  void render_fog_normal(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
      const fog_params& params) const;

  void render_reflect_colour(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
      const reflect_params& params, const GlFramebuffer& source) const;
  void render_reflect_normal(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
      const reflect_params& params) const;

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
