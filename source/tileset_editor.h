#ifndef TILESET_EDITOR_H
#define TILESET_EDITOR_H

#include "common.h"
#include "modal.h"

class Databank;
class Tileset;

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

};

#endif
