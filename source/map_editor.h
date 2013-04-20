#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "common.h"
#include "modal.h"
#include "vector.h"

class Databank;
class RenderUtil;
class CellMap;

class MapEditor: public Modal {
public:

  MapEditor(Databank& bank, RenderUtil& util, CellMap& map);

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  Databank& _bank;
  RenderUtil& _util;
  CellMap& _map;

  y::ivec2 _camera;

};

#endif
