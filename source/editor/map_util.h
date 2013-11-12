#ifndef EDITOR__MAP_UTIL_H
#define EDITOR__MAP_UTIL_H

#include "../cell.h"
#include "../common.h"
#include "../modal.h"
#include "../ui_util.h"
#include "../vector.h"

class CellMap;
class Databank;
class RenderUtil;

struct Zoom {
  static const y::vector<float> array;
};

// Action that adds a cell.
struct CellAddAction : public StackAction {
  CellAddAction(Databank& bank, CellMap& map,
                const y::ivec2& cell, const y::string& path);

  Databank& bank;
  CellMap& map;
  y::ivec2 cell;
  y::string path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that renames a cell.
struct CellRenameAction : public StackAction {
  CellRenameAction(Databank& bank, CellMap& map,
                   const y::ivec2& cell, const y::string& path);

  Databank& bank;
  CellMap& map;
  y::ivec2 cell;
  y::string new_path;
  mutable y::string old_path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that removes a cell.
struct CellRemoveAction : public StackAction {
  CellRemoveAction(Databank& bank, CellMap& map, const y::ivec2& cell);

  Databank& bank;
  CellMap& map;
  y::ivec2 cell;
  mutable y::string path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that changes a set of tiles.
struct TileEditAction : public StackAction {
  TileEditAction(CellMap& map, y::int32 layer);

  void set_tile(const y::ivec2& cell, const y::ivec2& tile, const Tile& t);

  CellMap& map;
  y::int32 layer;

  struct entry {
    Tile old_tile;
    Tile new_tile;
  };

  typedef y::pair<y::ivec2, y::ivec2> key;
  y::map<key, entry> edits;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that adds a script.
struct ScriptAddAction : public StackAction {
  ScriptAddAction(CellMap& map, const y::ivec2& min, const y::ivec2& max,
                  const y::string& path);

  CellMap& map;
  y::ivec2 min;
  y::ivec2 max;
  y::string path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that removes a script.
struct ScriptRemoveAction : public StackAction {
  ScriptRemoveAction(CellMap& map, const y::ivec2& min, const y::ivec2& max,
                     const y::string& path);

  CellMap& map;
  y::ivec2 min;
  y::ivec2 max;
  y::string path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that moves a script.
struct ScriptMoveAction : public StackAction {
  ScriptMoveAction(CellMap& map, const y::ivec2& min, const y::ivec2& max,
                   const y::ivec2& new_min, const y::ivec2& new_max,
                   const y::string& path);

  CellMap& map;
  y::ivec2 min;
  y::ivec2 max;
  y::ivec2 new_min;
  y::ivec2 new_max;
  y::string path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// The current set of tiles stored in the brush.
struct TileBrush {
  TileBrush();

  const Tile& get(const y::ivec2& v) const;
  /***/ Tile& get(const y::ivec2& v);

  y::ivec2 size;
  y::unique<Tile[]> array;
  static const y::ivec2 max_size;
};

// Displays the current contents of the brush on-screen.
class BrushPanel : public Panel {
public:

  BrushPanel(const Databank& bank, const TileBrush& brush);
  ~BrushPanel() override {}

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

private:

  const Databank& _bank;
  const TileBrush& _brush;

};

// Displays a list of Tilesets and allows picking tiles.
class TilePanel : public Panel {
public:

  TilePanel(const Databank& bank, TileBrush& brush);
  ~TilePanel() override {}

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

private:

  y::int32 get_list_height() const;

  void copy_drag_to_brush() const;

  const Databank& _bank;
  TileBrush& _brush;

  UiList _list;
  y::int32 _tileset_select;
  y::ivec2 _tile_hover;

};

// Displays a list of Scripts and allows picking them.
class ScriptPanel : public Panel {
public:

  ScriptPanel(const Databank& bank);
  ~ScriptPanel() override {}

  const y::string& get_script() const;
  void set_script(const y::string& path);

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

private:

  const Databank& _bank;
  UiList _list;
  y::int32 _script_select;

};

// Displays the available layers.
class LayerPanel : public Panel {
public:

  LayerPanel(const y::string_vector& status);
  ~LayerPanel() override {}

  y::int32 get_layer() const;

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

private:

  const y::string_vector& _status;
  UiList _list;
  y::int32 _layer_select;

};

// Displays the minimap.
class MinimapPanel : public Panel {
public:

  MinimapPanel(const CellMap& map, y::ivec2& camera, const y::int32& zoom);
  ~MinimapPanel() override {}

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

private:

  static const y::int32 scale;

  const CellMap& _map;
  y::ivec2& _camera;
  const y::int32& _zoom;

};

#endif
