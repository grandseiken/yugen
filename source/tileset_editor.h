#ifndef TILESET_EDITOR_H
#define TILESET_EDITOR_H

#include "common.h"
#include "modal.h"

class Databank;
class Tileset;

// Action that sets the collision of a tile.
struct SetCollideAction : public StackAction {
  SetCollideAction(Tileset& tileset, const y::ivec2& coord,
                   y::int32 old_collide, y::int32 new_collide);

  Tileset& tileset;
  y::ivec2 coord;
  y::int32 old_collide;
  y::int32 new_collide;

  virtual void redo() const;
  virtual void undo() const;
};

// Displays the tileset with collision data overlaid.
class TilesetPanel : public Panel {
public:

  TilesetPanel(Tileset& tileset, y::int32& collide_select,
               UndoStack& undo_stack);
  virtual ~TilesetPanel() {}

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  Tileset& _tileset;
  y::int32& _collide_select;
  UndoStack& _undo_stack;
  y::ivec2 _hover;

};

// Shows all the possible collision shapes.
class CollidePanel : public Panel {
public:

  static const y::int32 entries_per_row;

  CollidePanel(y::int32& collide_select);
  virtual ~CollidePanel() {}

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  y::int32& _collide_select;
  y::ivec2 _hover;

};

// Combines everything into an editor.
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
