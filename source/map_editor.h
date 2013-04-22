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

struct TileBrush {
  TileBrush();

  struct Entry {
    y::int32 tileset;
    y::int32 index;
  };

  y::ivec2 size;
  y::unique<Entry[]> array;
  static const y::int32 max_size = 16;
};

class BrushPanel : public Panel {
public:

  BrushPanel(const Databank& bank, const TileBrush& brush);
  virtual ~BrushPanel() {}

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  const Databank& _bank;
  const TileBrush& _brush;

};

class TilePanel : public Panel {
public:

  TilePanel(const Databank& bank, TileBrush& brush);
  virtual ~TilePanel() {}

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  y::int32 get_list_height() const;

  const Databank& _bank;
  TileBrush& _brush;
  y::int32 _tileset_select;
  y::ivec2 _tile_hover;

};

class LayerPanel : public Panel {
public:

  LayerPanel();
  virtual ~LayerPanel() {}

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  y::int8 _layer_select;

};

class MapEditor: public Modal {
public:

  MapEditor(Databank& bank, RenderUtil& util, CellMap& map);
  virtual ~MapEditor() {}

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

  TileBrush _tile_brush;
  BrushPanel _brush_panel;
  TilePanel _tile_panel;
  LayerPanel _layer_panel;

};

#endif
