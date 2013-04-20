#ifndef YEDIT_H
#define YEDIT_H

#include "common.h"
#include "modal.h"

class Databank;
class GlUtil;
class RenderUtil;
class Window;
namespace sf {
  class Event;
}

class Yedit : public Modal {
public:

  Yedit(Databank& bank, RenderUtil& util);
  virtual ~Yedit() {}

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  Databank& _bank;
  RenderUtil& _util;

  y::size _map_select;

};

#endif
