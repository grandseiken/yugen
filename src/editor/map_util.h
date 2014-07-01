#ifndef EDITOR_MAP_UTIL_H
#define EDITOR_MAP_UTIL_H

#include "../data/cell.h"
#include "../modal.h"
#include "../ui_util.h"
#include "../vec.h"

class CellMap;
class Databank;
class RenderUtil;

struct Zoom {
  static const std::vector<float> array;
};

// Action that adds a cell.
struct CellAddAction : public StackAction {
  CellAddAction(Databank& bank, CellMap& map,
                const y::ivec2& cell, const std::string& path);

  Databank& bank;
  CellMap& map;
  y::ivec2 cell;
  std::string path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that renames a cell.
struct CellRenameAction : public StackAction {
  CellRenameAction(Databank& bank, CellMap& map,
                   const y::ivec2& cell, const std::string& path);

  Databank& bank;
  CellMap& map;
  y::ivec2 cell;
  std::string new_path;
  mutable std::string old_path;

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
  mutable std::string path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that changes a set of tiles.
struct TileEditAction : public StackAction {
  TileEditAction(CellMap& map, std::int32_t layer);

  void set_tile(const y::ivec2& cell, const y::ivec2& tile, const Tile& t);

  CellMap& map;
  std::int32_t layer;

  struct entry {
    Tile old_tile;
    Tile new_tile;
  };

  typedef std::pair<y::ivec2, y::ivec2> key;
  std::unordered_map<key, entry> edits;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that adds a script.
struct ScriptAddAction : public StackAction {
  ScriptAddAction(CellMap& map, const y::ivec2& min, const y::ivec2& max,
                  const std::string& path);

  CellMap& map;
  y::ivec2 min;
  y::ivec2 max;
  std::string path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that removes a script.
struct ScriptRemoveAction : public StackAction {
  ScriptRemoveAction(CellMap& map, const y::ivec2& min, const y::ivec2& max,
                     const std::string& path);

  CellMap& map;
  y::ivec2 min;
  y::ivec2 max;
  std::string path;

  void redo() const override;
  void undo() const override;
  bool is_noop() const override;
};

// Action that moves a script.
struct ScriptMoveAction : public StackAction {
  ScriptMoveAction(CellMap& map, const y::ivec2& min, const y::ivec2& max,
                   const y::ivec2& new_min, const y::ivec2& new_max,
                   const std::string& path);

  CellMap& map;
  y::ivec2 min;
  y::ivec2 max;
  y::ivec2 new_min;
  y::ivec2 new_max;
  std::string path;

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
  std::unique_ptr<Tile[]> array;
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

  std::int32_t get_list_height() const;

  void copy_drag_to_brush() const;

  const Databank& _bank;
  TileBrush& _brush;

  UiList _list;
  std::int32_t _tileset_select;
  y::ivec2 _tile_hover;

};

// Displays a list of Scripts and allows picking them.
class ScriptPanel : public Panel {
public:

  ScriptPanel(const Databank& bank);
  ~ScriptPanel() override {}

  const std::string& get_script() const;
  void set_script(const std::string& path);

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

private:

  const Databank& _bank;
  UiList _list;
  std::int32_t _script_select;

};

// Displays the available layers.
class LayerPanel : public Panel {
public:

  LayerPanel(const std::vector<std::string>& status);
  ~LayerPanel() override {}

  std::int32_t get_layer() const;

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

private:

  const std::vector<std::string>& _status;
  UiList _list;
  std::int32_t _layer_select;

};

// Displays the minimap.
class MinimapPanel : public Panel {
public:

  MinimapPanel(const CellMap& map, y::ivec2& camera, const std::int32_t& zoom);
  ~MinimapPanel() override {}

  bool event(const sf::Event& e) override;
  void update() override;
  void draw(RenderUtil& util) const override;

private:

  static const std::int32_t scale;

  const CellMap& _map;
  y::ivec2& _camera;
  const std::int32_t& _zoom;

};

#endif
