#include "map_util.h"

#include "../data/bank.h"
#include "../data/tileset.h"
#include "../render/util.h"
#include "../render/window.h"

#include <SFML/Window.hpp>

const y::vector<float> Zoom::array{.25f, .5f, 1.f, 2.f, 3.f, 4.f};

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

bool CellAddAction::is_noop() const
{
  return path.substr(0, 7) != "/world/" ||
      path.substr(path.length() - 5) != ".cell" ||
      map.get_coord(cell);
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

bool CellRenameAction::is_noop() const
{
  return new_path.substr(0, 7) != "/world/" ||
      new_path.substr(new_path.length() - 5) != ".cell" ||
      !map.get_coord(cell) ||
      bank.cells.get_name(*map.get_coord(cell)) == new_path ||
      bank.cells.is_name_used(new_path);
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

bool CellRemoveAction::is_noop() const
{
  return !map.get_coord(cell);
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

bool TileEditAction::is_noop() const
{
  for (const auto& pair : edits) {
    if (pair.second.old_tile != pair.second.new_tile) {
      return false;
    }
  }
  return true;
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
  map.add_script({min, max, path});
}

void ScriptAddAction::undo() const
{
  map.remove_script({min, max, path});
}

bool ScriptAddAction::is_noop() const
{
  return map.has_script({min, max, path});
}

ScriptRemoveAction::ScriptRemoveAction(
    CellMap& map, const y::ivec2& min, const y::ivec2& max,
    const y::string& path)
  : map(map)
  , min(min)
  , max(max)
  , path(path)
{
}

void ScriptRemoveAction::redo() const
{
  map.remove_script({min, max, path});
}

void ScriptRemoveAction::undo() const
{
  map.add_script({min, max, path});
}

bool ScriptRemoveAction::is_noop() const
{
  return !map.has_script({min, max, path});
}

ScriptMoveAction::ScriptMoveAction(
    CellMap& map, const y::ivec2& min, const y::ivec2& max,
    const y::ivec2& new_min, const y::ivec2& new_max, const y::string& path)
  : map(map)
  , min(min)
  , max(max)
  , new_min(new_min)
  , new_max(new_max)
  , path(path)
{
}

void ScriptMoveAction::redo() const
{
  map.remove_script({min, max, path});
  map.add_script({new_min, new_max, path});
}

void ScriptMoveAction::undo() const
{
  map.remove_script({new_min, new_max, path});
  map.add_script({min, max, path});
}

bool ScriptMoveAction::is_noop() const
{
  return map.has_script({new_min, new_max, path}) ||
      !map.has_script({min, max, path});
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

    batch.iadd_sprite(e.tileset->get_texture().texture, Tileset::tile_size,
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

  const GlTexture2D& tex =
      _bank.tilesets.get(_tileset_select).get_texture().texture;
  y::int32 tx_max = RenderUtil::to_grid(tex.get_size())[xx];
  y::int32 ty_max = y::min(y::size(7), _bank.tilesets.size());

  set_size(tex.get_size() + RenderUtil::from_grid() * y::ivec2{0, ty_max});
  _list.set_size({tx_max, ty_max});
}

void TilePanel::draw(RenderUtil& util) const
{
  // Render panel.
  const Tileset& t = _bank.tilesets.get(_tileset_select);
  const GlTexture2D& tex = t.get_texture().texture;

  _list.draw(util, _bank.tilesets.get_names(), _tileset_select);

  // Render tileset.
  util.irender_fill({0, RenderUtil::from_grid(_list.get_size())[yy]},
                    tex.get_size(), colour::dark_panel);
  util.irender_sprite(tex, tex.get_size(),
                      {0, RenderUtil::from_grid(_list.get_size())[yy]},
                      y::ivec2(), 0.f, colour::white);

  // Render hover.
  y::ivec2 start = is_dragging() ? get_drag_start() : _tile_hover;
  y::ivec2 min = y::min(start, _tile_hover);
  y::ivec2 max = y::max(start, _tile_hover);

  if (y::in_region(_tile_hover, y::ivec2(), t.get_size()) || is_dragging()) {
    min = y::max(min, {0, 0});
    max = y::min(max, t.get_size() - y::ivec2{1, 1});
    util.irender_outline(
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

LayerPanel::LayerPanel(const y::vector<y::string>& status)
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
  static y::vector<y::string> layer_names{
    "1 Background",
    "2 Collision",
    "3 Foreground",
    "4 Scripts"};

  _list.draw(util, layer_names, Cell::background_layers + _layer_select);

  y::int32 i = 0;
  for (const y::string& s : _status) {
    util.irender_text(s, RenderUtil::from_grid({0, 4 + i++}), colour::item);
  }
}

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
  util.irender_fill(y::ivec2(), get_size(), colour::dark_panel);
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
        batch.iadd_sprite(t.tileset->get_texture().texture, Tileset::tile_size,
                          u, t.tileset->from_index(t.index), d, colour::white);
      }
    }
  }
  util.render_batch(batch);

  // Draw viewport.
  const Resolution& r = util.get_window().get_mode();
  y::ivec2 vmin =
      _camera - y::ivec2(y::fvec2(r.size) / (2.f * Zoom::array[_zoom])) -
      Tileset::tile_size * min * Cell::cell_size;
  y::ivec2 vmax = vmin + y::ivec2(y::fvec2(r.size) / Zoom::array[_zoom]);

  vmin = y::max(vmin, y::ivec2());
  vmax = y::min(vmax, (max - min) * Cell::cell_size * Tileset::tile_size);
  if (vmin < vmax) {
    util.irender_outline(vmin, vmax - vmin, colour::outline);
  }
  util.set_scale(1.f);
}

const y::int32 MinimapPanel::scale = 16;
