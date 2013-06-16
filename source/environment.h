#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "common.h"
#include "gl_util.h"

class RenderUtil;

class Environment : public y::no_copy {
public:

  Environment(GlUtil& util);

  void render(RenderUtil& util,
              const GlFramebuffer& colourbuffer,
              const GlFramebuffer& normalbuffer) const;

private:

  GlProgram _fog_program;

};

#endif
