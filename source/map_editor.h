#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "common.h"
#include "modal.h"
#include "vector.h"

class Databank;
class RenderBatch;
class RenderUtil;
class CellCoord;
class CellMap;

class MapEditor: public Modal {
public:

  MapEditor(Databank& bank, RenderUtil& util, CellMap& map);

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  y::ivec2 world_to_camera(const y::ivec2& v) const;
  y::ivec2 camera_to_world(const y::ivec2& v) const;

  void draw_cell_layer(
      RenderBatch& batch, const CellCoord& coord, y::int8 layer) const;

  Databank& _bank;
  RenderUtil& _util;
  CellMap& _map;

  y::ivec2 _camera;

};

#endif
