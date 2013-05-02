#ifndef ACTOR_H
#define ACTOR_H

#include "common.h"

class Script;

class Actor : public y::no_copy {
public:

  Actor(Script& script);

private:

  Script& _script;

};

#endif
