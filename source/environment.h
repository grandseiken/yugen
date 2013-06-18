#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "common.h"
#include "gl_handle.h"

class GlUtil;
class RenderUtil;

class Environment : public y::no_copy {
public:

  Environment(GlUtil& util);

  void render(
      RenderUtil& util, const y::wvec2& camera_min, const y::wvec2& camera_max,
      const GlFramebuffer& colourbuffer,
      const GlFramebuffer& normalbuffer) const;

private:

  GlProgram _fog_program;
  GlTexture2D _f2d_256;
  GlTexture3D _f3d_128;

  // TODO: temp. Get rid of this.
  mutable y::int32 _frame;

};

#endif
