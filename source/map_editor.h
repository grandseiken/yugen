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

// Action that changes a set of tiles.
struct TileEditAction : public StackAction {
  TileEditAction(CellMap& map, y::int32 layer);

  CellMap& map;
  y::int32 layer;

  struct Entry {
    Tile old_tile;
    Tile new_tile;
  };

  typedef y::pair<y::ivec2, y::ivec2> Key;
  y::map<Key, Entry> edits;

  virtual void redo() const;
  virtual void undo() const;
};

// Action that sets a script. Set path to empty string to remove a script.
struct ScriptSetAction : public StackAction {
  ScriptSetAction(CellMap& map, const y::ivec2& cell, const y::ivec2& tile,
                  const y::string& new_path, const y::string& old_path);

  CellMap& map;
  y::ivec2 cell;
  y::ivec2 tile;
  y::string new_path;
  y::string old_path;

  virtual void redo() const;
  virtual void undo() const;
};

// The current set of tiles stored in the brush.
struct TileBrush {
  TileBrush();

  y::ivec2 size;
  y::unique<Tile[]> array;
  static const y::int32 max_size = 16;
};

// Displays the current contents of the brush on-screen.
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

// Displays a list of Tilesets and allows picking tiles.
class TilePanel : public Panel {
public:

  TilePanel(const Databank& bank, TileBrush& brush);
  virtual ~TilePanel() {}

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

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
  virtual ~ScriptPanel() {}

  const y::string& get_script() const;
  void set_script(const y::string& path);

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  const Databank& _bank;
  UiList _list;
  y::int32 _script_select;

};

// Displays the available layers.
class LayerPanel : public Panel {
public:

  LayerPanel(const y::string_vector& status);
  virtual ~LayerPanel() {}

  y::int32 get_layer() const;

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  const y::string_vector& _status;
  UiList _list;
  y::int32 _layer_select;

};

// Displays the minimap.
class MinimapPanel : public Panel {
public:

  MinimapPanel(const CellMap& map, y::ivec2& camera, const y::int32& zoom);
  virtual ~MinimapPanel() {}

  virtual bool event(const sf::Event& e);
  virtual void update();
  virtual void draw(RenderUtil& util) const;

private:

  static const y::int32 scale;

  const CellMap& _map;
  y::ivec2& _camera;
  const y::int32& _zoom;

};

// Combines the above Panels and draws the world underneath.
class MapEditor: public Modal {
public:

  MapEditor(Databank& bank, RenderUtil& util, CellMap& map);
  virtual ~MapEditor() {}

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  friend class MinimapPanel;

  y::ivec2 world_to_camera(const y::ivec2& v) const;
  y::ivec2 camera_to_world(const y::ivec2& v) const;

  void copy_drag_to_brush();
  void copy_brush_to_map() const;

  void draw_cell_layer(
      RenderBatch& batch, const y::ivec2& coord, y::int32 layer) const;

  Databank& _bank;
  RenderUtil& _util;
  CellMap& _map;

  bool _camera_drag;
  y::int32 _zoom;
  y::ivec2 _camera;
  y::ivec2 _hover;
  y::ivec2 _hover_cell;
  y::ivec2 _hover_tile;
  y::string_vector _layer_status;

  TileBrush _tile_brush;
  BrushPanel _brush_panel;
  TilePanel _tile_panel;
  ScriptPanel _script_panel;
  LayerPanel _layer_panel;
  MinimapPanel _minimap_panel;

  y::unique<TileEditAction> _tile_edit_action;

  TextInputResult _input_result;

};

#endif
