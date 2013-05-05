#ifndef TILESET_EDITOR_H
#define TILESET_EDITOR_H

#include "common.h"
#include "modal.h"

class Databank;
class Tileset;

class TilesetPanel : public Panel {
public:

  TilesetPanel(Tileset& tileset, y::int32& collide_select);
  virtual ~TilesetPanel() {}

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  Tileset& _tileset;
  y::int32& _collide_select;
  y::ivec2 _hover;

};

class CollidePanel : public Panel {
public:

  static const y::int32 entries_per_row = 8;

  CollidePanel(y::int32& collide_select);
  virtual ~CollidePanel() {}

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  y::int32& _collide_select;
  y::ivec2 _hover;

};

class TilesetEditor : public Modal {
public:

  TilesetEditor(Databank& bank, RenderUtil& util, Tileset& tileset);
  virtual ~TilesetEditor() {}

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  Databank& _bank;
  RenderUtil& _util;
  Tileset& _tileset;

  y::int32 _collide_select;

  TilesetPanel _tileset_panel;
  CollidePanel _collide_panel;

};

#endif
