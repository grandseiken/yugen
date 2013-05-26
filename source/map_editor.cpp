#include "map_editor.h"

#include "databank.h"
#include "render_util.h"
#include "tileset.h"
#include "window.h"

#include <SFML/Window.hpp>

CellAddAction::CellAddAction(Databank& bank, CellMap& map,
                             const y::ivec2& cell, const y::string& path)
  : bank(bank)
  , map(map)
  , cell(cell)
  , path(path)
{
}

void CellAddAction::redo() const
{
  if (!bank.cells.is_name_used(path)) {
    bank.cells.insert(path, y::move_unique(new CellBlueprint()));
  }
  map.set_coord(cell, bank.cells.get(path));
}

void CellAddAction::undo() const
{
  map.clear_coord(cell);
}

CellRenameAction::CellRenameAction(Databank& bank, CellMap& map,
                                   const y::ivec2& cell, const y::string& path)
  : bank(bank)
  , map(map)
  , cell(cell)
  , new_path(path)
{
}

void CellRenameAction::redo() const
{
  old_path = bank.cells.get_name(*map.get_coord(cell));
  bank.cells.rename(*map.get_coord(cell), new_path);
}

void CellRenameAction::undo() const
{
  bank.cells.rename(*map.get_coord(cell), old_path);
}

CellRemoveAction::CellRemoveAction(Databank& bank, CellMap& map,
                                   const y::ivec2& cell)
  : bank(bank)
  , map(map)
  , cell(cell)
{
}

void CellRemoveAction::redo() const
{
  path = bank.cells.get_name(*map.get_coord(cell));
  map.clear_coord(cell);
}

void CellRemoveAction::undo() const
{
  map.set_coord(cell, bank.cells.get(path));
}

TileEditAction::TileEditAction(CellMap& map, y::int32 layer)
  : map(map)
  , layer(layer)
{
}

void TileEditAction::set_tile(
    const y::ivec2& cell, const y::ivec2& tile, const Tile& t)
{
  const auto& p = y::make_pair(cell, tile);

  auto it = edits.find(p);
  if (it == edits.end()) {
    auto& edit = edits[p];
    edit.old_tile = map.get_coord(cell)->get_tile(layer, tile);
    it = edits.find(p);
  }
  it->second.new_tile = t;
}

void TileEditAction::redo() const
{
  for (const auto& pair : edits) {
    CellBlueprint* cell = map.get_coord(pair.first.first);
    cell->set_tile(layer, pair.first.second, pair.second.new_tile);
  }
}

void TileEditAction::undo() const
{
  for (const auto& pair : edits) {
    CellBlueprint* cell = map.get_coord(pair.first.first);
    cell->set_tile(layer, pair.first.second, pair.second.old_tile);
  }
}

ScriptAddAction::ScriptAddAction(
    CellMap& map, const y::ivec2& min, const y::ivec2& max,
    const y::string& path)
  : map(map)
  , min(min)
  , max(max)
  , path(path)
{
}

void ScriptAddAction::redo() const
{
  map.add_script(min, max, path);
}

void ScriptAddAction::undo() const
{
  map.remove_script(map.get_script_index_at(min));
}

ScriptRemoveAction::ScriptRemoveAction(CellMap& map, const y::ivec2& world)
  : map(map)
  , world(world)
{
}

void ScriptRemoveAction::redo() const
{
  const ScriptBlueprint& s = map.get_script_at(world);
  min = s.min;
  max = s.max;
  path = s.path;
  map.remove_script(map.get_script_index_at(world));
}

void ScriptRemoveAction::undo() const
{
  map.add_script(min, max, path);
}

ScriptMoveAction::ScriptMoveAction(CellMap& map,
                                   const y::ivec2& start, const y::ivec2& end)
  : map(map)
  , start(start)
  , end(end)
{
}

void ScriptMoveAction::redo() const
{
  const ScriptBlueprint& s = map.get_script_at(start);
  y::ivec2 min = s.min;
  y::ivec2 max = s.max;
  y::string path = s.path;
  map.remove_script(map.get_script_index_at(start));
  map.add_script(min + end - start, max + end - start, path);
}

void ScriptMoveAction::undo() const
{
  const ScriptBlueprint& s = map.get_script_at(end);
  y::ivec2 min = s.min;
  y::ivec2 max = s.max;
  y::string path = s.path;
  map.remove_script(map.get_script_index_at(end));
  map.add_script(min + start - end, max + start - end, path);
}

TileBrush::TileBrush()
  : size{1, 1}
  , array(new Tile[max_size[xx] * max_size[yy]])
{
  auto& entry = array[0];
  entry.tileset = 0;
  entry.index = 0;
}

const Tile& TileBrush::get(const y::ivec2& v) const
{
  return array[v[xx] + v[yy] * max_size[xx]];
}

Tile& TileBrush::get(const y::ivec2& v)
{
  return array[v[xx] + v[yy] * max_size[xx]];
}

const y::ivec2 TileBrush::max_size{16, 16};

BrushPanel::BrushPanel(const Databank& bank, const TileBrush& brush)
  : Panel(y::ivec2(), y::ivec2())
  , _bank(bank)
  , _brush(brush)
{
}

bool BrushPanel::event(const sf::Event& e)
{
  return e.type == sf::Event::MouseMoved;
}

void BrushPanel::update()
{
  y::ivec2 tiles = y::max({1, 1}, _brush.size);
  set_size(tiles * Tileset::tile_size);
}

void BrushPanel::draw(RenderUtil& util) const
{
  if (!(_brush.size > y::ivec2())) {
    return;
  }

  RenderBatch batch;
  for (auto it = y::cartesian(_brush.size); it; ++it) {
    const Tile& e = _brush.get(*it);
    if (!e.tileset) {
      continue;
    }

    batch.add_sprite(e.tileset->get_texture(), Tileset::tile_size,
                     *it * Tileset::tile_size, e.tileset->from_index(e.index),
                     0.f, colour::white);
  }
  util.render_batch(batch);
}

TilePanel::TilePanel(const Databank& bank, TileBrush& brush)
  : Panel(y::ivec2(), y::ivec2())
  , _bank(bank)
  , _brush(brush)
  , _list(y::ivec2(), y::ivec2(), colour::panel, colour::item, colour::select)
  , _tileset_select(0)
  , _tile_hover{-1, -1}
{
}

bool TilePanel::event(const sf::Event& e)
{
  // Update hover position.
  if (e.type == sf::Event::MouseMoved) {
    if (e.mouseMove.y < RenderUtil::from_grid(_list.get_size())[yy]) {
      _tile_hover = {-1, -1};
    }
    else {
      _tile_hover = {e.mouseMove.x, e.mouseMove.y -
                     RenderUtil::from_grid(_list.get_size())[yy]};
      _tile_hover = _tile_hover.euclidean_div(Tileset::tile_size);
    }
    return true;
  }
  if (e.type == sf::Event::MouseLeft) {
    _tile_hover = {-1, -1};
  }

  // Start drag.
  if (e.type == sf::Event::MouseButtonPressed &&
      (e.mouseButton.button == sf::Mouse::Left ||
       e.mouseButton.button == sf::Mouse::Right)) {
    start_drag(_tile_hover);
  }

  // End drag and update brush.
  if (e.type == sf::Event::MouseButtonReleased && is_dragging() &&
      (e.mouseButton.button == sf::Mouse::Left ||
       e.mouseButton.button == sf::Mouse::Right)) {
    copy_drag_to_brush();
    stop_drag();
  }

  // Change tileset.
  if (e.type == sf::Event::MouseWheelMoved && !is_dragging()) {
    _tileset_select -= e.mouseWheel.delta;
    return true;
  }
  if (e.type != sf::Event::KeyPressed || is_dragging()) {
    return false;
  }

  switch (e.key.code) {
    case sf::Keyboard::Q:
      --_tileset_select;
      break;
    case sf::Keyboard::E:
      ++_tileset_select;
      break;
    default: {}
  }
  return false;
}

void TilePanel::update()
{
  y::roll<y::int32>(_tileset_select, 0, _bank.tilesets.size());

  const GlTexture& tex = _bank.tilesets.get(_tileset_select).get_texture();
  y::int32 tx_max = RenderUtil::to_grid(tex.get_size())[xx];
  y::int32 ty_max = y::min(y::size(7), _bank.tilesets.size());

  set_size(tex.get_size() + RenderUtil::from_grid() * y::ivec2{0, ty_max});
  _list.set_size({tx_max, ty_max});
}

void TilePanel::draw(RenderUtil& util) const
{
  // Render panel.
  const Tileset& t = _bank.tilesets.get(_tileset_select);
  const GlTexture& tex = t.get_texture();

  _list.draw(util, _bank.tilesets.get_names(), _tileset_select);

  // Render tileset.
  util.render_fill({0, RenderUtil::from_grid(_list.get_size())[yy]},
                   tex.get_size(), colour::dark_panel);
  util.render_sprite(tex, tex.get_size(),
                     {0, RenderUtil::from_grid(_list.get_size())[yy]},
                     y::ivec2(), 0.f, colour::white);

  // Render hover.
  y::ivec2 start = is_dragging() ? get_drag_start() : _tile_hover;
  y::ivec2 min = y::min(start, _tile_hover);
  y::ivec2 max = y::max(start, _tile_hover);

  if (_tile_hover.in_region(y::ivec2(), t.get_size()) || is_dragging()) {
    min = y::max(min, {0, 0});
    max = y::min(max, t.get_size() - y::ivec2{1, 1});
    util.render_outline(
        min * Tileset::tile_size +
            y::ivec2{0, RenderUtil::from_grid(_list.get_size())[yy]},
        (y::ivec2{1, 1} + (max - min)) * Tileset::tile_size, colour::hover);
  }
}

void TilePanel::copy_drag_to_brush() const
{
  const Tileset& t = _bank.tilesets.get(_tileset_select);

  y::ivec2 min = y::min(get_drag_start(), _tile_hover);
  y::ivec2 max = y::max(get_drag_start(), _tile_hover);
  min = y::max(min, {0, 0});
  max = y::min(max, t.get_size() - y::ivec2{1, 1});
  max = y::min(max, min + TileBrush::max_size - y::ivec2{1, 1});
  _brush.size = y::ivec2{1, 1} + max - min;
  for (auto it = y::cartesian(_brush.size); it; ++it) {
    Tile& entry = _brush.get(*it);
    entry.tileset = &_bank.tilesets.get(_tileset_select);
    entry.index = t.to_index(*it + min);
  }
}

ScriptPanel::ScriptPanel(const Databank& bank)
  : Panel(y::ivec2(), y::ivec2())
  , _bank(bank)
  , _list(y::ivec2(), y::ivec2(), colour::panel, colour::item, colour::select)
  , _script_select(0)
{
}

const y::string& ScriptPanel::get_script() const
{
  static const y::string& missing = "/yedit/missing.lua";
  if (y::size(_script_select) >= _bank.scripts.size()) {
    return missing;
  }
  return _bank.scripts.get(_script_select).path;
}

void ScriptPanel::set_script(const y::string& path)
{
  y::int32 index = _bank.scripts.get_index(path);
  if (index >= 0) {
    _script_select = index;
  }
}

bool ScriptPanel::event(const sf::Event& e)
{
  // Update hover position.
  if (e.type == sf::Event::MouseMoved) {
    return true;
  }

  // Change tileset.
  if (e.type == sf::Event::MouseWheelMoved) {
    _script_select -= e.mouseWheel.delta;
    return true;
  }

  if (e.type != sf::Event::KeyPressed) {
    return false;
  }
  switch (e.key.code) {
    case sf::Keyboard::Q:
      --_script_select;
      break;
    case sf::Keyboard::E:
      ++_script_select;
      break;
    default: {}
  }
  return false;
}

void ScriptPanel::update()
{
  y::roll<y::int32>(_script_select, 0, _bank.scripts.size());

  y::ivec2 v = {32, y::min(17, y::int32(_bank.scripts.size()))};
  set_size(RenderUtil::from_grid(v));
  _list.set_size(v);
}

void ScriptPanel::draw(RenderUtil& util) const
{
  _list.draw(util, _bank.scripts.get_names(), _script_select);
}

LayerPanel::LayerPanel(const y::string_vector& status)
  : Panel(y::ivec2(), y::ivec2())
  , _status(status)
  , _list(y::ivec2(), y::ivec2(), colour::panel, colour::item, colour::select)
  , _layer_select(0)
{
}

y::int32 LayerPanel::get_layer() const
{
  return _layer_select;
}

bool LayerPanel::event(const sf::Event& e)
{
  if (e.type == sf::Event::MouseMoved) {
    return true;
  }
  if (e.type == sf::Event::MouseWheelMoved) {
    _layer_select -= e.mouseWheel.delta;
    return true;
  }
  if (e.type != sf::Event::KeyPressed) {
    return false;
  }

  switch (e.key.code) {
    case sf::Keyboard::Num1:
      _layer_select = -1;
      break;
    case sf::Keyboard::Num2:
      _layer_select = 0;
      break;
    case sf::Keyboard::Num3:
      _layer_select = 1;
      break;
    case sf::Keyboard::Num4:
      _layer_select = 2;
      break;
    case sf::Keyboard::Tab:
      ++_layer_select;
      break;
    default: {}
  }
  return false;
}

void LayerPanel::update()
{
  y::roll<y::int32>(_layer_select, -Cell::background_layers,
                   2 + Cell::foreground_layers);
  y::size len = 12;
  for (const y::string& s : _status) {
    len = y::max(len, s.length());
  }
  set_size(RenderUtil::from_grid({
      y::int32(len), 4 + y::int32(_status.size())}));
  _list.set_size({y::int32(len), 4});
}

void LayerPanel::draw(RenderUtil& util) const
{
  static y::string_vector layer_names{
    "1 Background",
    "2 Collision",
    "3 Foreground",
    "4 Scripts"};

  _list.draw(util, layer_names, Cell::background_layers + _layer_select);

  y::int32 i = 0;
  for (const y::string& s : _status) {
    util.render_text(s, RenderUtil::from_grid({0, 4 + i++}), colour::item);
  }
}

static const y::vector<float> zoom_array{.25f, .5f, 1.f, 2.f, 3.f, 4.f};

MinimapPanel::MinimapPanel(const CellMap& map,
                           y::ivec2& camera, const y::int32& zoom)
  : Panel(y::ivec2(), y::ivec2())
  , _map(map)
  , _camera(camera)
  , _zoom(zoom)
{
}

bool MinimapPanel::event(const sf::Event& e)
{
  // Start drag.
  if (e.type == sf::Event::MouseButtonPressed &&
      (e.mouseButton.button == sf::Mouse::Left ||
       e.mouseButton.button == sf::Mouse::Right)) {
    start_drag(y::ivec2());
  }
  if (e.type == sf::Event::MouseButtonReleased && is_dragging() &&
      (e.mouseButton.button == sf::Mouse::Left ||
       e.mouseButton.button == sf::Mouse::Right)) {
    stop_drag();
  }

  // Update camera position.
  if ((e.type == sf::Event::MouseMoved ||
       e.type == sf::Event::MouseButtonPressed) && is_dragging()) {
    y::ivec2 v = e.type == sf::Event::MouseMoved ?
        y::ivec2{e.mouseMove.x, e.mouseMove.y} :
        y::ivec2{e.mouseButton.x, e.mouseButton.y};
    _camera = v * scale +
        _map.get_boundary_min() * Cell::cell_size * Tileset::tile_size;
  }
  if (e.type == sf::Event::MouseMoved) {
    return true;
  }
  return false;
}

void MinimapPanel::update()
{
  set_size((_map.get_boundary_max() - _map.get_boundary_min()) *
           (Tileset::tile_size / scale) * Cell::cell_size);
}

void MinimapPanel::draw(RenderUtil& util) const
{
  util.render_fill(y::ivec2(), get_size(), colour::dark_panel);
  util.set_scale(1.f / scale);
  const y::ivec2& min = _map.get_boundary_min();
  const y::ivec2& max = _map.get_boundary_max();
  y::ivec2 c;
  RenderBatch batch;
  for (y::int32 layer = -Cell::background_layers;
       layer <= Cell::foreground_layers; ++layer) {
    for (auto it = _map.get_cartesian(); it; ++it) {
      const y::ivec2& c = *it;
      if (!_map.is_coord_used(c)) {
        continue;
      }
      const CellBlueprint* cell = _map.get_coord(c);

      for (auto it = y::cartesian(Cell::cell_size); it; ++it) {
        const y::ivec2& v = *it;
        const Tile& t = cell->get_tile(layer, v);
        // Draw the tile.
        if (!t.tileset) {
          continue;
        }

        float d = .8f - layer * .1f;
        y::ivec2 u = Tileset::tile_size * (v + (c - min) * Cell::cell_size);
        batch.add_sprite(t.tileset->get_texture(), Tileset::tile_size,
                         u, t.tileset->from_index(t.index), d, colour::white);
      }
    }
  }
  util.render_batch(batch);

  // Draw viewport.
  const Resolution& r = util.get_window().get_mode();
  y::ivec2 vmin =
      _camera - y::ivec2(y::fvec2(r.size) / (2.f * zoom_array[_zoom])) -
      Tileset::tile_size * min * Cell::cell_size;
  y::ivec2 vmax = vmin + y::ivec2(y::fvec2(r.size) / zoom_array[_zoom]);

  vmin = y::max(vmin, y::ivec2());
  vmax = y::min(vmax, (max - min) * Cell::cell_size * Tileset::tile_size);
  if (vmin < vmax) {
    util.render_outline(vmin, vmax - vmin, colour::outline);
  }
  util.set_scale(1.f);
}

const y::int32 MinimapPanel::scale = 16;

MapEditor::MapEditor(Databank& bank, RenderUtil& util, CellMap& map)
  : _bank(bank)
  , _util(util)
  , _map(map)
  , _camera_drag(false)
  , _zoom(2)
  , _hover{-1, -1}
  , _brush_panel(bank, _tile_brush)
  , _tile_panel(bank, _tile_brush)
  , _script_panel(bank)
  , _layer_panel(_layer_status)
  , _minimap_panel(map, _camera, _zoom)
{
  get_panel_ui().add(_brush_panel);
  get_panel_ui().add(_tile_panel);
  get_panel_ui().add(_script_panel);
  get_panel_ui().add(_layer_panel);
  get_panel_ui().add(_minimap_panel);
}

void MapEditor::event(const sf::Event& e)
{
  // Update hover position.
  if (e.type == sf::Event::MouseMoved) {
    _hover = y::ivec2(y::fvec2(float(e.mouseMove.x), float(e.mouseMove.y)) /
                      zoom_array[_zoom]);
  }
  if (e.type == sf::Event::MouseLeft) {
    _hover = {-1, -1};
  }
  if (e.type == sf::Event::MouseWheelMoved) {
    _layer_panel.event(e);
  }

  bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
               sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
  bool control = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

  // Start camera drag.
  if (e.type == sf::Event::MouseButtonPressed && !is_dragging() &&
      (shift || e.mouseButton.button == sf::Mouse::Middle)) {
    start_drag(_hover);
    _camera_drag = true;
  }
  else if (e.type == sf::Event::MouseButtonPressed && !is_dragging() &&
           (e.mouseButton.button == sf::Mouse::Left ||
            e.mouseButton.button == sf::Mouse::Right)) {
    // Start tile draw or pick.
    if (!is_script_layer()) {
      start_drag(camera_to_world(_hover).euclidean_div(Tileset::tile_size));
      if (e.mouseButton.button == sf::Mouse::Left) {
        _tile_edit_style = control;
        _tile_edit_action = y::move_unique(
            new TileEditAction(_map, _layer_panel.get_layer()));
      }
    }
    // Start script region create or move.
    else if (e.mouseButton.button == sf::Mouse::Left) {
      start_drag(get_hover_world());
      if (control || !_map.has_script_at(get_hover_world())) {
        _script_add_action = y::move_unique(
            new ScriptAddAction(_map, get_hover_world(), get_hover_world(),
                _script_panel.get_script()));
      }
    }
    // Pick script.
    else if (e.mouseButton.button == sf::Mouse::Right &&
             _map.has_script_at(get_hover_world())) {
      _script_panel.set_script(_map.get_script_at(get_hover_world()).path);
    }
  }

  if (e.type == sf::Event::MouseButtonReleased && is_dragging()) {
    // End draw and commit tile edit.
    if (_tile_edit_action) {
      get_undo_stack().new_action(y::move_unique(_tile_edit_action));
    }
    // End drag and add script.
    else if (_script_add_action) {
      get_undo_stack().new_action(y::move_unique(_script_add_action));
    }
    else if (is_script_layer()) {
      get_undo_stack().new_action(y::move_unique(
          new ScriptMoveAction(_map, get_drag_start(), get_hover_world())));
    }
    // End pick and copy tiles.
    else if (!_camera_drag) {
      copy_drag_to_brush();
    }
    stop_drag();
    _camera_drag = false;
  }

  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  CellBlueprint* cell = _map.get_coord(get_hover_cell());
  y::int32 old_zoom = _zoom;
  switch (e.key.code) {
    // Quit!
    case sf::Keyboard::Escape:
      end();
      break;

    // Kick off game.
    case sf::Keyboard::Return:
      if (shift) {
        y::string call = "bin/yugen " + _bank.maps.get_name(_map) +
                         " " + y::to_string(get_hover_world()[xx]) +
                         " " + y::to_string(get_hover_world()[yy]);
        std::cout << call << " exited with code " << system(call.c_str()) <<
            std::endl;
      }
      break;

    // Rename cell.
    case sf::Keyboard::R:
      if (is_mouse_on_screen() && cell) {
        const y::string& name =
            _bank.cells.get_name(*cell);
        push(y::move_unique(new TextInputModal(
            _util, name, _input_result, "Rename cell " + name + " to:")));
      }
      break;

    // New cell.
    case sf::Keyboard::N:
      if (is_mouse_on_screen() && !cell) {
        push(y::move_unique(new TextInputModal(
            _util, "/world/new.cell", _input_result, "Add cell using name:")));
      }
      break;

    // Remove cell or script.
    case sf::Keyboard::Delete:
      if (!is_script_layer() && _map.is_coord_used(get_hover_cell())) {
        get_undo_stack().new_action(y::move_unique(
            new CellRemoveAction(_bank, _map, get_hover_cell())));
      }
      if (is_script_layer() && _map.has_script_at(get_hover_world())) {
        get_undo_stack().new_action(y::move_unique(
            new ScriptRemoveAction(_map, get_hover_world())));
      }
      break;

    // Zoom in.
    case sf::Keyboard::Z:
      ++_zoom;
      if (_zoom >= 0 && _zoom < y::int32(zoom_array.size())) {
        _hover = y::ivec2(y::fvec2(_hover) *
                          zoom_array[old_zoom] / zoom_array[_zoom]);
      }
      break;

    // Zoom out.
    case sf::Keyboard::X:
      --_zoom;
      if (_zoom >= 0 && _zoom < y::int32(zoom_array.size())) {
        _hover = y::ivec2(y::fvec2(_hover) *
                          zoom_array[old_zoom] / zoom_array[_zoom]);
      }
      break;

    // Unused key.
    default: {}
  }
}

void MapEditor::update()
{
  y::clamp(_zoom, 0, y::int32(zoom_array.size()));

  // Layout UI.
  const Resolution& r = _util.get_window().get_mode();
  const y::ivec2 spacing = RenderUtil::from_grid({0, 1});

  bool script_layer = _layer_panel.get_layer() > Cell::foreground_layers;
  _tile_panel.set_visible(!script_layer);
  _brush_panel.set_visible(!script_layer);
  _script_panel.set_visible(script_layer);
  Panel& top_panel = script_layer ?
      (Panel&)_script_panel : (Panel&)_tile_panel;

  top_panel.set_origin(RenderUtil::from_grid());
  _layer_panel.set_origin(top_panel.get_origin() + spacing +
                          y::ivec2{0, top_panel.get_size()[yy]});
  _brush_panel.set_origin(_layer_panel.get_origin() + spacing +
                          y::ivec2{0, _layer_panel.get_size()[yy]});
  _minimap_panel.set_origin(r.size - _minimap_panel.get_size() -
                            RenderUtil::from_grid());

  // Move camera with arrow keys.
  y::int32 speed = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) ? 32 : 8;
  y::ivec2 d;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
    --d[yy];
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
    ++d[yy];
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
    --d[xx];
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
    ++d[xx];
  }
  _camera += speed * d;

  // Update hover and status text.
  if (!is_mouse_on_screen()) {
    return;
  }

  _layer_status.clear();
  y::sstream ss;
  ss << get_hover_cell() << " : " << get_hover_tile() <<
      " [" << zoom_array[_zoom] << "X]";
  _layer_status.emplace_back(ss.str());
  if (_map.is_coord_used(get_hover_cell())) {
    _layer_status.emplace_back(
        _bank.cells.get_name(*_map.get_coord(get_hover_cell())));
  }

  // Drag camera.
  if (_camera_drag) {
    _camera += get_drag_start() - _hover;
    start_drag(_hover);
    return;
  }

  // Rename or add cell.
  if (_input_result.success) {
    const y::string& result = _input_result.result;
    if (result.substr(0, 7) == "/world/" &&
        result.substr(result.length() - 5) == ".cell") {
      if (_map.is_coord_used(get_hover_cell())) {
        if (!_bank.cells.is_name_used(result)) {
          get_undo_stack().new_action(y::move_unique(
              new CellRenameAction(_bank, _map, get_hover_cell(), result)));
        }
      }
      else {
        get_undo_stack().new_action(y::move_unique(
            new CellAddAction(_bank, _map, get_hover_cell(), result)));
      }
    }
    _input_result.success = false;
  }

  if (!is_dragging()) {
    return;
  }
  // Continuous tile drawing.
  if (_tile_edit_action) {
    copy_brush_to_action();
  }
  // Script sizing.
  if (_script_add_action) {
    _script_add_action->min = y::min(get_drag_start(), get_hover_world());
    _script_add_action->max = y::max(get_drag_start(), get_hover_world());
  }
}

void MapEditor::draw() const
{
  _util.set_scale(zoom_array[_zoom]);

  // Draw cell contents.
  for (y::int32 layer = -Cell::background_layers;
       layer <= Cell::foreground_layers; ++layer) {
    RenderBatch batch;
    for (auto it = _map.get_cartesian(); it; ++it) {
      draw_cell_layer(batch, *it, layer);
    }
    _util.render_batch(batch);
    _util.get_gl().bind_window(false, true);
  }

  // Draw cell-grid.
  y::ivec2 min = y::min(_map.get_boundary_min(), get_hover_cell());
  y::ivec2 max = y::max(
      _map.get_boundary_max(), y::ivec2{1, 1} + get_hover_cell());
  for (auto it = y::cartesian(min, max); it; ++it) {
    if (_map.is_coord_used(*it) ||
        (is_mouse_on_screen() && *it == get_hover_cell())) {
      _util.render_outline(
          world_to_camera(*it * Tileset::tile_size * Cell::cell_size),
          Tileset::tile_size * Cell::cell_size, colour::panel);
    }
  }

  // Draw script regions.
  if (is_script_layer()) {
    draw_scripts();
  }

  // Draw native resolution indicator.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
    const Resolution& r = _util.get_window().get_mode();
    _util.render_outline(r.size / 2 - RenderUtil::native_size / 2,
                         RenderUtil::native_size, colour::outline);
  }

  // Draw cursor.
  draw_cursor();

  // Draw UI.
  _util.set_scale(1.f);
  get_panel_ui().draw(_util);
}

y::ivec2 MapEditor::world_to_camera(const y::ivec2& v) const
{
  const Resolution& r = _util.get_window().get_mode();
  return v - _camera + y::ivec2(y::fvec2(r.size / 2) / zoom_array[_zoom]);
}

y::ivec2 MapEditor::camera_to_world(const y::ivec2& v) const
{
  const Resolution& r = _util.get_window().get_mode();
  return v - y::ivec2(y::fvec2(r.size / 2) / zoom_array[_zoom]) + _camera;
}

void MapEditor::copy_drag_to_brush()
{
  y::ivec2 min = y::min(get_drag_start(), get_hover_world());
  y::ivec2 max = y::max(get_drag_start(), get_hover_world());
  for (auto it = y::cartesian(min, y::ivec2{1, 1} + max); it; ++it) {
    y::ivec2 u = *it - min;
    if (!(u < TileBrush::max_size)) {
      continue;
    }

    y::ivec2 c = it->euclidean_div(Cell::cell_size);
    y::ivec2 t = it->euclidean_mod(Cell::cell_size);
    Tile& e = _tile_brush.get(u);

    if (!_map.is_coord_used(c)) {
      e.tileset = y::null;
      e.index = 0;
    }
    else {
      CellBlueprint* cell = _map.get_coord(c);
      const Tile& tile = cell->get_tile(_layer_panel.get_layer(), t);
      e.tileset = tile.tileset;
      e.index = tile.index;
    }
  }
  _tile_brush.size = y::min(y::ivec2{1, 1} + max - min, TileBrush::max_size);
}

void MapEditor::copy_brush_to_action() const
{
  y::ivec2 min;
  y::ivec2 max = _tile_brush.size;
  y::ivec2 flip{0, 0};
  if (_tile_edit_style) {
    _tile_edit_action->edits.clear();
    min = y::min(get_drag_start(), get_hover_world());
    max = y::ivec2{1, 1} + y::max(get_drag_start(), get_hover_world());
    flip = {get_drag_start()[xx] > get_hover_world()[xx],
            get_drag_start()[yy] > get_hover_world()[yy]};
  }

  for (auto it = y::cartesian(min, max); it; ++it) {
    y::ivec2 v = _tile_edit_style ? *it : *it + get_hover_world();
    y::ivec2 c = v.euclidean_div(Cell::cell_size);
    y::ivec2 t = v.euclidean_mod(Cell::cell_size);
    if (!_map.is_coord_used(c)) {
      continue;
    }

    y::ivec2 u = _tile_edit_style ?
        (*it - min) * (y::ivec2{1, 1} - flip) + (*it - max) * flip :
        *it + get_hover_world() - get_drag_start();
    _tile_edit_action->set_tile(
        c, t, _tile_brush.get(u.euclidean_mod(_tile_brush.size)));
  }
}

void MapEditor::draw_cell_layer(
    RenderBatch& batch, const y::ivec2& coord, y::int32 layer) const
{
  if (!_map.is_coord_used(coord)) {
    return;
  }
  const CellBlueprint* cell = _map.get_coord(coord);

  y::int32 active_layer = _layer_panel.get_layer();
  const y::fvec4& c = active_layer > Cell::foreground_layers ? colour::white :
      layer < active_layer ? colour::dark :
      layer == active_layer ? colour::white :
      colour::transparent;

  bool edit = is_dragging() &&
      _tile_edit_action && _tile_edit_action->layer == layer;

  for (auto it = y::cartesian(Cell::cell_size); it; ++it) {
    // In the middle of a TileEditAction we need to render the
    // uncommited drawing.
    y::map<TileEditAction::key, TileEditAction::entry>::const_iterator jt;
    if (edit) {
      jt = _tile_edit_action->edits.find(y::make_pair(coord, *it));
    }
    const Tile& t = edit && jt != _tile_edit_action->edits.end() ?
        jt->second.new_tile : cell->get_tile(layer, *it);

    // Draw the tile.
    if (!t.tileset) {
      continue;
    }

    y::ivec2 camera = world_to_camera(
        Tileset::tile_size * (*it + coord * Cell::cell_size));

    batch.add_sprite(t.tileset->get_texture(), Tileset::tile_size,
                     camera, t.tileset->from_index(t.index), 0.f, c);
  }
}

void MapEditor::draw_scripts() const
{
  // Script region the mouse is hovering over.
  const ScriptBlueprint* hover_script =
      !is_dragging() && _map.has_script_at(get_hover_world()) ?
          &_map.get_script_at(get_hover_world()) : y::null;

  // Script being dragged.
  const ScriptBlueprint* drag_script = is_dragging() && !_script_add_action ?
      &_map.get_script_at(get_drag_start()) : y::null;

  for (const ScriptBlueprint& s : _map.get_scripts()) {
    // Need to show an uncommitted script drag.
    y::ivec2 off = &s == drag_script ?
        get_hover_world() - get_drag_start() : y::ivec2();
    y::ivec2 min = world_to_camera((s.min + off) * Tileset::tile_size);
    y::ivec2 max = world_to_camera((s.max + off) * Tileset::tile_size);
    _util.render_fill(min, Tileset::tile_size + max - min,
                      colour::highlight);
    _util.render_outline(min, Tileset::tile_size + max - min,
                         colour::outline);
    _util.render_text(
        s.path, min, &s == hover_script || &s == drag_script ?
            colour::select : colour::item);
  }
}

void MapEditor::draw_cursor() const
{
  if (is_dragging() && !_script_add_action && is_script_layer()) {
    return;
  }

  y::ivec2 cursor_end = get_hover_world() + _tile_brush.size - y::ivec2{1, 1};
  // Draw a rectangle-dragging-out cursor.
  if (is_dragging() && !_camera_drag &&
      (!_tile_edit_action || _tile_edit_style)) {
    y::ivec2 min = y::min(get_drag_start(), get_hover_world());
    y::ivec2 max = y::max(get_drag_start(), get_hover_world());
    _util.render_outline(
        world_to_camera(min * Tileset::tile_size),
        Tileset::tile_size * (y::ivec2{1, 1} + max - min), colour::hover);
  }
  // Draw a fixed-size cursor from the top-left.
  else if (is_mouse_on_screen() &&
           (_map.is_coord_used(get_hover_cell()) ||
            _map.is_coord_used(cursor_end.euclidean_div(Cell::cell_size)))) {
    _util.render_outline(
        world_to_camera(get_hover_world() * Tileset::tile_size),
        Tileset::tile_size * (is_script_layer() ?
                              y::ivec2{1, 1} : _tile_brush.size),
        colour::hover);
  }
}

bool MapEditor::is_script_layer() const
{
  return _layer_panel.get_layer() > Cell::foreground_layers;
}

bool MapEditor::is_mouse_on_screen() const
{
  return _hover >= y::ivec2();
}

y::ivec2 MapEditor::get_hover_world() const
{
  return camera_to_world(_hover).euclidean_div(Tileset::tile_size);
}

y::ivec2 MapEditor::get_hover_tile() const
{
  return get_hover_world().euclidean_mod(Cell::cell_size);
}

y::ivec2 MapEditor::get_hover_cell() const
{
  return get_hover_world().euclidean_div(Cell::cell_size);
}
