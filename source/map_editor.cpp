#include "map_editor.h"

#include "cell.h"
#include "databank.h"
#include "render_util.h"
#include "tileset.h"
#include "window.h"

#include <SFML/Window.hpp>

TileBrush::TileBrush()
  : size{1, 1}
  , array(new Tile[max_size * max_size])
{
  auto& entry = array[0];
  entry.tileset = 0;
  entry.index = 0;
}

TileEditAction::TileEditAction(CellMap& map, y::int32 layer)
  : map(map)
  , layer(layer)
{
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
  if (!_brush.size[xx] || !_brush.size[yy]) {
    return;
  }

  RenderBatch batch;
  for (y::ivec2 v; v[xx] < _brush.size[xx]; ++v[xx]) {
    for (v[yy] = 0; v[yy] < _brush.size[yy]; ++v[yy]) {
      const Tile& e =
          _brush.array[v[xx] + v[yy] * TileBrush::max_size];
      if (!e.tileset) {
        continue;
      }

      batch.add_sprite(e.tileset->get_texture(), Tileset::tile_size,
                       v * Tileset::tile_size, e.tileset->from_index(e.index),
                       0.f, Colour::white);
    }
  }
  util.render_batch(batch);
}

TilePanel::TilePanel(const Databank& bank, TileBrush& brush)
  : Panel(y::ivec2(), y::ivec2())
  , _bank(bank)
  , _brush(brush)
  , _list(y::ivec2(), y::ivec2(), Colour::panel, Colour::item, Colour::select)
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
      _tile_hover /= Tileset::tile_size;
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
                   tex.get_size(), Colour::dark_panel);
  util.render_sprite(tex, tex.get_size(),
                     {0, RenderUtil::from_grid(_list.get_size())[yy]},
                     y::ivec2(), 0.f, Colour::white);

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
        (y::ivec2{1, 1} + (max - min)) * Tileset::tile_size, Colour::hover);
  }
}

void TilePanel::copy_drag_to_brush() const
{
  const Tileset& t = _bank.tilesets.get(_tileset_select);

  y::ivec2 min = y::min(get_drag_start(), _tile_hover);
  y::ivec2 max = y::max(get_drag_start(), _tile_hover);
  min = y::max(min, {0, 0});
  max = y::min(max, t.get_size() - y::ivec2{1, 1});
  max = y::min(max, min + y::ivec2{TileBrush::max_size - 1,
                                   TileBrush::max_size - 1});
  _brush.size = y::ivec2{1, 1} + max - min;
  for (y::int32 x = 0; x < _brush.size[xx]; ++x) {
    for (y::int32 y = 0; y < _brush.size[yy]; ++y) {
      Tile& entry = _brush.array[x + y * TileBrush::max_size];
      entry.tileset = &_bank.tilesets.get(_tileset_select);
      entry.index = t.to_index(y::ivec2{x, y} + min);
    }
  }
}

LayerPanel::LayerPanel(const y::string_vector& status)
  : Panel(y::ivec2(), y::ivec2())
  , _status(status)
  , _list(y::ivec2(), y::ivec2(), Colour::panel, Colour::item, Colour::select)
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
    "4 Actors"};

  _list.draw(util, layer_names, Cell::background_layers + _layer_select);

  y::int32 i = 0;
  for (const y::string& s : _status) {
    util.render_text_grid(s, {0, 4 + i++}, Colour::item);
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
  util.render_fill(y::ivec2(), get_size(), Colour::dark_panel);
  util.set_scale(1.f / scale);
  const y::ivec2& min = _map.get_boundary_min();
  const y::ivec2& max = _map.get_boundary_max();
  y::ivec2 c;
  for (y::int32 layer = -Cell::background_layers;
       layer <= Cell::foreground_layers; ++layer) {
    // TODO: get depth working so we can move this outside.
    RenderBatch batch;
    for (c[xx] = min[xx]; c[xx] < max[xx]; ++c[xx]) {
      for (c[yy] = min[yy]; c[yy] < max[yy]; ++c[yy]) {
        if (!_map.is_coord_used(c)) {
          continue;
        }
        const CellBlueprint* cell = _map.get_coord(c);

        y::ivec2 v;
        for (; v[xx] < Cell::cell_size[xx]; ++v[xx]) {
          for (v[yy] = 0; v[yy] < Cell::cell_size[yy]; ++v[yy]) {
            const Tile& t = cell->get_tile(layer, v);
            // Draw the tile.
            if (!t.tileset) {
              continue;
            }

            y::ivec2 u = Tileset::tile_size * (v + (c - min) * Cell::cell_size);
            batch.add_sprite(t.tileset->get_texture(), Tileset::tile_size,
                             u, t.tileset->from_index(t.index),
                             (layer + Cell::background_layers) /
                                 (1 + Cell::background_layers +
                                  Cell::foreground_layers), Colour::white);
          }
        }
      }
    }
    util.render_batch(batch);
    util.get_gl().bind_window(false, true);
  }
  // Draw viewport.
  const Resolution& r = util.get_window().get_mode();
  y::ivec2 vmin =
      _camera - y::ivec2(y::fvec2(r.size) / (2.f * zoom_array[_zoom])) -
      Tileset::tile_size * min * Cell::cell_size;
  y::ivec2 vmax = vmin + y::ivec2(y::fvec2(r.size) / zoom_array[_zoom]);

  vmin = y::max(vmin, y::ivec2());
  vmax = y::min(vmax, (max - min) * Cell::cell_size * Tileset::tile_size);
  if (vmin[xx] < vmax[xx] && vmin[yy] < vmax[yy]) {
    util.render_outline(vmin, vmax - vmin, Colour::outline);
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
  , _layer_panel(_layer_status)
  , _minimap_panel(map, _camera, _zoom)
{
  get_panel_ui().add(_brush_panel);
  get_panel_ui().add(_tile_panel);
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

  // Start camera drag.
  if (e.type == sf::Event::MouseButtonPressed && !is_dragging() &&
      (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
       sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) ||
       e.mouseButton.button == sf::Mouse::Middle)) {
    start_drag(_hover);
    _camera_drag = true;
  }
  // Start draw or pick.
  // TODO: control + draw should draw a rectangle.
  else if (e.type == sf::Event::MouseButtonPressed && !is_dragging() &&
           (e.mouseButton.button == sf::Mouse::Left ||
            e.mouseButton.button == sf::Mouse::Right) &&
           _layer_panel.get_layer() <= Cell::foreground_layers) {
    start_drag(camera_to_world(_hover).euclidean_div(Tileset::tile_size));
    if (e.mouseButton.button == sf::Mouse::Left) {
      _tile_edit_action = y::move_unique(
          new TileEditAction(_map, _layer_panel.get_layer()));
    }
  }

  if (e.type == sf::Event::MouseButtonReleased && is_dragging()) {
    // End draw and commit tile edit.
    if (_tile_edit_action) {
      get_undo_stack().new_action(y::move_unique(_tile_edit_action));
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

  y::int32 old_zoom = _zoom;
  switch (e.key.code) {
    // Quit!
    case sf::Keyboard::Escape:
      end();
      break;
    // Rename cell.
    // TODO: make undoable.
    case sf::Keyboard::R:
      if (_hover[xx] >= 0 && _hover[yy] >= 0 &&
          _map.is_coord_used(_hover_cell)) {
        const y::string& name =
            _bank.cells.get_name(*_map.get_coord(_hover_cell));
        push(y::move_unique(new TextInputModal(
            _util, name, _input_result, "Rename cell " + name + " to:")));
      }
      break;
    // New cell.
    // TODO: make undoable.
    case sf::Keyboard::N:
      if (_hover[xx] >= 0 && _hover[yy] >= 0 &&
          !_map.is_coord_used(_hover_cell)) {
        push(y::move_unique(new TextInputModal(
            _util, "/world/new.cell", _input_result, "Add cell using name:")));
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
    default: {}
  }
}

void MapEditor::update()
{
  y::clamp(_zoom, 0, y::int32(zoom_array.size()));

  // Layout UI.
  const Resolution& r = _util.get_window().get_mode();
  const y::ivec2 spacing = RenderUtil::from_grid({0, 1});

  _tile_panel.set_origin(RenderUtil::from_grid());
  _layer_panel.set_origin(_tile_panel.get_origin() + spacing +
                          y::ivec2{0, _tile_panel.get_size()[yy]});
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

  if (_hover[xx] < 0 || _hover[yy] < 0) {
    return;
  }

  // Update hover and status text.
  y::ivec2 tile = camera_to_world(_hover).euclidean_div(Tileset::tile_size);
  _hover_cell = tile.euclidean_div(Cell::cell_size);
  _hover_tile = tile.euclidean_mod(Cell::cell_size);

  _layer_status.clear();
  y::sstream ss;
  ss << _hover_cell[xx] << ", " << _hover_cell[yy] <<
      " : " << _hover_tile[xx] << ", " << _hover_tile[yy] <<
      " [" << zoom_array[_zoom] << "X]";
  _layer_status.push_back(ss.str());
  if (_map.is_coord_used(_hover_cell)) {
    _layer_status.push_back(_bank.cells.get_name(*_map.get_coord(_hover_cell)));
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
    if (_map.is_coord_used(_hover_cell)) {
      _bank.cells.rename(*_map.get_coord(_hover_cell), result);
    }
    else {
      if (!_bank.cells.is_name_used(result)) {
        _bank.cells.insert(result, y::move_unique(new CellBlueprint()));
      }
      _map.set_coord(_hover_cell, _bank.cells.get(result));
    }
    _input_result.success = false;
  }

  // Continuous tile drawing.
  if (!is_dragging() || !_tile_edit_action) {
    return;
  }
  copy_brush_to_map();
}

void MapEditor::draw() const
{
  _util.get_gl().bind_window(true, true);
  _util.set_scale(zoom_array[_zoom]);

  // Draw cells.
  y::ivec2 min = _map.get_boundary_min();
  y::ivec2 max = _map.get_boundary_max();
  y::ivec2 c;
  for (y::int32 layer = -Cell::background_layers;
       layer <= Cell::foreground_layers; ++layer) {
    RenderBatch batch;
    for (c[xx] = min[xx]; c[xx] < max[xx]; ++c[xx]) {
      for (c[yy] = min[yy]; c[yy] < max[yy]; ++c[yy]) {
        draw_cell_layer(batch, c, layer);
      }
    }
    _util.render_batch(batch);
    _util.get_gl().bind_window(false, true);
  }

  // Draw cell-grid.
  min = y::min(min, _hover_cell);
  max = y::max(max, y::ivec2{1, 1} + _hover_cell);
  for (c[xx] = min[xx]; c[xx] < max[xx]; ++c[xx]) {
    for (c[yy] = min[yy]; c[yy] < max[yy]; ++c[yy]) {
      if (_map.is_coord_used(c) ||
          (c == _hover_cell && _hover[xx] >= 0 && _hover[yy] >= 0)) {
        _util.render_outline(
            world_to_camera(c * Tileset::tile_size * Cell::cell_size),
            Tileset::tile_size * Cell::cell_size, Colour::panel);
      }
    }
  }

  // Draw native resolution indicator.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
    const Resolution& r = _util.get_window().get_mode();
    _util.render_outline(r.size / 2 - RenderUtil::native_size / 2,
                         RenderUtil::native_size, Colour::outline);
  }

  // Draw cursor.
  y::ivec2 t =
      (_hover_tile + _hover_cell * Cell::cell_size);
  if (is_dragging() && !_tile_edit_action && !_camera_drag) {
    y::ivec2 u = get_drag_start();
    y::ivec2 min = y::min(t, u);
    y::ivec2 max = y::max(t, u);
    _util.render_outline(
        world_to_camera(min * Tileset::tile_size),
        Tileset::tile_size * (y::ivec2{1, 1} + max - min), Colour::hover);
  }
  else if (_hover[xx] >= 0 && _hover[yy] >= 0 &&
           _map.is_coord_used(_hover_cell)) {
    _util.render_outline(world_to_camera(t * Tileset::tile_size),
                         Tileset::tile_size * _tile_brush.size, Colour::hover);
  }

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
  y::ivec2 t = _hover_tile + _hover_cell * Cell::cell_size;
  y::ivec2 u = get_drag_start();
  y::ivec2 min = y::min(t, u);
  y::ivec2 max = y::max(t, u);
  y::ivec2 v;
  for (v[xx] = min[xx]; v[xx] <= max[xx]; ++v[xx]) {
    for (v[yy] = min[yy]; v[yy] <= max[yy]; ++v[yy]) {
      u = v - min;
      if (u[xx] >= TileBrush::max_size || u[yy] >= TileBrush::max_size) {
        continue;
      }
      y::ivec2 c = v.euclidean_div(Cell::cell_size);
      t = v.euclidean_mod(Cell::cell_size);
      Tile& e = _tile_brush.array[u[xx] + u[yy] * TileBrush::max_size];
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
  }
  _tile_brush.size = y::min(y::ivec2{1, 1} + max - min,
                            {TileBrush::max_size, TileBrush::max_size});
}

void MapEditor::copy_brush_to_map() const
{
  y::ivec2 tile = camera_to_world(_hover).euclidean_div(Tileset::tile_size);
  y::int32 layer = _tile_edit_action->layer;
  y::ivec2 start = get_drag_start();
  y::ivec2 v;
  for (; v[xx] < _tile_brush.size[xx]; ++v[xx]) {
    for (v[yy] = 0; v[yy] < _tile_brush.size[yy]; ++v[yy]) {
      y::ivec2 c = (tile + v).euclidean_div(Cell::cell_size);
      y::ivec2 t = (tile + v).euclidean_mod(Cell::cell_size);
      if (!_map.is_coord_used(c)) {
        continue;
      }

      CellBlueprint* cell = _map.get_coord(c);
      y::ivec2 vt = (tile + v - start).euclidean_mod(_tile_brush.size);

      const Tile& e = _tile_brush.array[vt[xx] + vt[yy] * TileBrush::max_size];
      const auto& p = y::make_pair(c, t);

      auto it = _tile_edit_action->edits.find(p);
      if (it == _tile_edit_action->edits.end()) {
        auto& edit = _tile_edit_action->edits[p];
        edit.old_tile = cell->get_tile(layer, t);
        it = _tile_edit_action->edits.find(p);
      }
      it->second.new_tile.tileset = e.tileset;
      it->second.new_tile.index = e.index;
    }
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
  const Colour& c = active_layer > Cell::foreground_layers ? Colour::white :
      layer < active_layer ? Colour::dark :
      layer == active_layer ? Colour::white :
      Colour::transparent;

  bool edit = is_dragging() &&
      _tile_edit_action && _tile_edit_action->layer == layer;

  y::ivec2 v;
  for (; v[xx] < Cell::cell_size[xx]; ++v[xx]) {
    for (v[yy] = 0; v[yy] < Cell::cell_size[yy]; ++v[yy]) {
      // In the middle of a TileEditAction we need to render the
      // uncommited drawing.
      y::map<TileEditAction::Key, TileEditAction::Entry>::const_iterator it;
      if (edit) {
        it = _tile_edit_action->edits.find(y::make_pair(coord, v));
      }
      const Tile& t = edit && it != _tile_edit_action->edits.end() ?
          it->second.new_tile : cell->get_tile(layer, v);

      // Draw the tile.
      if (!t.tileset) {
        continue;
      }

      y::ivec2 camera = world_to_camera(
          Tileset::tile_size * (v + coord * Cell::cell_size));

      batch.add_sprite(t.tileset->get_texture(), Tileset::tile_size,
                       camera, t.tileset->from_index(t.index),
                       (layer + Cell::background_layers) /
                           (1 + Cell::background_layers +
                            Cell::foreground_layers), c);
    }
  }
}
