#ifndef EDITOR_TILESET_H
#define EDITOR_TILESET_H

#include "../modal.h"
#include "../vec.h"

class Databank;
class Filesystem;
class Tileset;

// Action that sets the collision of a tile.
struct SetCollideAction : public StackAction {
  SetCollideAction(Tileset& tileset, const y::ivec2& coord,
                   y::int32 old_collide, y::int32 new_collide);

  Tileset& tileset;
  y::ivec2 coord;
  y::int32 old_collide;
  y::int32 new_collide;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Displays the tileset with collision data overlaid.
class TilesetPanel : public Panel {
public:

  TilesetPanel(Tileset& tileset, y::int32& collide_select,
               UndoStack& undo_stack);
  ~TilesetPanel() override {}

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

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
  ~CollidePanel() override {}

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

private:

  y::int32& _collide_select;
  y::ivec2 _hover;

};

// Combines everything into an editor.
class TilesetEditor : public Modal {
public:

  TilesetEditor(
      Filesystem& output, Databank& bank, RenderUtil& util, Tileset& tileset);
  ~TilesetEditor() override {}

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  Filesystem& _output;
  Databank& _bank;
  RenderUtil& _util;
  Tileset& _tileset;

  y::int32 _collide_select;

  TilesetPanel _tileset_panel;
  CollidePanel _collide_panel;

};

#endif
