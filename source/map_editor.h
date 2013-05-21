#ifndef MAP_EDITOR_H
#define MAP_EDITOR_H

#include "cell.h"
#include "common.h"
#include "modal.h"
#include "ui_util.h"
#include "vector.h"

class CellMap;
class Databank;
class RenderBatch;
class RenderUtil;

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
};

// Action that removes a script.
struct ScriptRemoveAction : public StackAction {
  ScriptRemoveAction(CellMap& map, const y::ivec2& world);

  CellMap& map;
  y::ivec2 world;
  mutable y::ivec2 min;
  mutable y::ivec2 max;
  mutable y::string path;

  void redo() const override;
  void undo() const override;
};

// Action that moves a script.
struct ScriptMoveAction : public StackAction {
  ScriptMoveAction(CellMap& map, const y::ivec2& start, const y::ivec2& end);

  CellMap& map;
  y::ivec2 start;
  y::ivec2 end;

  void redo() const override;
  void undo() const override;
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

// Combines the classes above into a map editor.
class MapEditor: public Modal {
public:

  MapEditor(Databank& bank, RenderUtil& util, CellMap& map);
  ~MapEditor() override {}

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  friend class MinimapPanel;

  y::ivec2 world_to_camera(const y::ivec2& v) const;
  y::ivec2 camera_to_world(const y::ivec2& v) const;

  void copy_drag_to_brush();
  void copy_brush_to_action() const;

  void draw_cell_layer(
      RenderBatch& batch, const y::ivec2& coord, y::int32 layer) const;
  void draw_scripts() const;
  void draw_cursor() const;

  // Helper accessors.
  bool is_script_layer() const;
  bool is_mouse_on_screen() const;
  y::ivec2 get_hover_world() const;
  y::ivec2 get_hover_tile() const;
  y::ivec2 get_hover_cell() const;

  Databank& _bank;
  RenderUtil& _util;
  CellMap& _map;

  bool _camera_drag;
  y::int32 _zoom;
  y::ivec2 _camera;
  y::ivec2 _hover;
  y::string_vector _layer_status;

  TileBrush _tile_brush;
  BrushPanel _brush_panel;
  TilePanel _tile_panel;
  ScriptPanel _script_panel;
  LayerPanel _layer_panel;
  MinimapPanel _minimap_panel;

  bool _tile_edit_style;
  y::unique<TileEditAction> _tile_edit_action;
  y::unique<ScriptAddAction> _script_add_action;
  TextInputResult _input_result;

};

#endif
