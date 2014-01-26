#ifndef EDITOR_MAP_H
#define EDITOR_MAP_H

#include "map_util.h"

#include "../common/memory.h"
#include "../render/gl_handle.h"
#include "../modal.h"
#include "../vector.h"

#include <random>

class CellMap;
class Databank;
class RenderBatch;
class RenderUtil;

// Combines the classes from map_editor_util.h into a map editor.
class MapEditor: public Modal {
public:

  MapEditor(Filesystem& output, Databank& bank, RenderUtil& util, CellMap& map);
  ~MapEditor() override {}

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  friend class MinimapPanel;

  // Enumeration of the 'handles' we have for moving scripts.
  enum script_drag {
    SCRIPT_UL,
    SCRIPT_U,
    SCRIPT_UR,
    SCRIPT_L,
    SCRIPT_MOVE,
    SCRIPT_R,
    SCRIPT_DL,
    SCRIPT_D,
    SCRIPT_DR,
  };
  script_drag get_script_drag(const ScriptBlueprint& blueprint,
                              const y::ivec2& v) const;
  void get_script_drag_result(y::ivec2& min_output, y::ivec2& max_output,
                              const ScriptBlueprint& blueprint, script_drag sd,
                              bool drag_modify, bool handle_modify) const;

  y::ivec2 world_to_camera(const y::ivec2& v) const;
  y::ivec2 camera_to_world(const y::ivec2& v) const;

  void copy_drag_to_brush();
  void copy_brush_to_action() const;

  const y::fvec4& colour_for_layer(y::int32 layer) const;
  void draw_cell_layer(
      RenderBatch& batch,
      const y::ivec2& coord, y::int32 layer, bool normal) const;
  void draw_scripts() const;
  void draw_cursor() const;

  // Helper accessors.
  bool is_script_layer() const;
  bool is_mouse_on_screen() const;
  y::ivec2 get_hover_world() const;
  y::ivec2 get_hover_tile() const;
  y::ivec2 get_hover_cell() const;

  Filesystem& _output;
  Databank& _bank;
  RenderUtil& _util;
  CellMap& _map;
  GlUnique<GlProgram> _yedit_scene_program;

  bool _camera_drag;
  bool _light_drag;
  y::int32 _zoom;
  y::ivec2 _camera;
  y::ivec2 _hover;
  y::fvec2 _light_direction;
  y::vector<y::string> _layer_status;

  TileBrush _tile_brush;
  BrushPanel _brush_panel;
  TilePanel _tile_panel;
  ScriptPanel _script_panel;
  LayerPanel _layer_panel;
  MinimapPanel _minimap_panel;

  bool _tile_edit_style;
  y::unique<TileEditAction> _tile_edit_action;
  y::unique<ScriptAddAction> _script_add_action;
  script_drag _script_drag;
  TextInputResult _input_result;
  ConfirmationResult _confirm_result;

  std::default_random_engine _generator;

};

#endif
