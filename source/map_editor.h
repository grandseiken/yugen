#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "common.h"
#include "modal.h"
#include "vector.h"

class MapEditor: public Modal {
public:

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  y::ivec2 _camera;

};

#endif
