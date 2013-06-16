#include "environment.h"
#include "perlin.h"
#include "render_util.h"

Environment::Environment(GlUtil& gl)
  : _fog_program(gl.make_program({
      "/shaders/fog.v.glsl",
      "/shaders/fog.f.glsl"}))
{
  typedef Perlin<float> fperlin;
  typedef Perlin<y::fvec2> fv2perlin;

  fperlin fp;
  fperlin::field f2d_256;
  fp.generate_perlin<2>(f2d_256, 16, 16, 4);

  // TODO: test.
  fv2perlin fv2p;
  (void)fv2p;
}

void Environment::render(RenderUtil& util,
                         const GlFramebuffer& colourbuffer,
                         const GlFramebuffer& normalbuffer) const
{
  util.get_gl().enable_depth(false);
  util.get_gl().enable_blend(false);

  // TODO: render some fog using perlin buffers. (This is sorta broken.)
  colourbuffer.bind(true, false);
  glClearColor(.5f, .2f, .2f, .4f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.f, 0.f, 0.f, 0.f);

  // TODO: make a function that uses glClear for this.
  normalbuffer.bind(true, false);
  util.irender_fill(y::ivec2(), normalbuffer.get_size(),
                    y::fvec4{.5f, .5f, 0.f, 1.f});
}
