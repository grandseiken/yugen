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

private:

  GlUnique<GlProgram> _fog_program;
  GlUnique<GlTexture2D> _f2d_256;
  GlUnique<GlTexture3D> _f3d_128;

  // TODO: temp. Get rid of this.
  mutable y::int32 _frame;

};

#endif
