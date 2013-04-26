#include "map_editor.h"

#include "cell.h"
#include "databank.h"
#include "render_util.h"
#include "tileset.h"
#include "window.h"

#include <SFML/Window.hpp>

const Colour c_panel(.2f, .2f, .2f, .7f);
const Colour c_dark_panel(0.f, 0.f, 0.f, .7f);
const Colour c_item(.6f, .6f, .6f, 1.f);
const Colour c_hover(.9f, .9f, .9f, .5f);
const Colour c_select(.9f, .9f, .9f, 1.f);

// TODO: take this out into a nice happy class.
void render_list_grid(
    RenderUtil& util,
    const Colour& panel, const y::vector<Colour>& items,
    const y::string_vector& source, y::size select,
    const y::ivec2& origin, const y::ivec2& size)
{
  util.render_fill_grid(origin, size, panel);

  y::int32 w = size[xx];
  y::int32 h = size[yy];
  y::int32 offset = y::max<y::int32>(0,
    select >= source.size() - h / 2 - 1 ? source.size() - h : select - h / 2);

  for (y::int32 i = 0; i < h; ++i) {
    if (i + offset >= y::int32(items.size()) ||
        i + offset >= y::int32(source.size())) {
      break;
    }

    y::string s = source[i + offset];
    s = y::int32(s.length()) > w ? s.substr(0, w - 3) + "..." : s;
    util.render_text_grid(s, origin + y::ivec2{0, i}, items[i + offset]);
  }
}

// TODO: likewise.
void render_list_grid(
    RenderUtil& util,
    const Colour& panel, const Colour& item, const Colour& s_item,
    const y::string_vector& source, y::size select,
    const y::ivec2& origin, const y::ivec2& size)
{
  y::vector<Colour> items;
  for (y::size n = 0; n < source.size(); ++n) {
    items.push_back(n == select ? s_item : item);
  }

  render_list_grid(util, panel, items, source, select, origin, size);
}

TileBrush::TileBrush()
  : size{1, 1}
  , array(new Entry[max_size * max_size])
{
  auto& entry = array[0];
  entry.tileset = 0;
  entry.index = 0;
}

TileEditAction::TileEditAction(CellMap& map, y::int8 layer)
  : map(map)
  , layer(layer)
{
}

void TileEditAction::redo() const
{
}

void TileEditAction::undo() const
{
}

BrushPanel::BrushPanel(const Databank& bank, const TileBrush& brush)
  : Panel(y::ivec2(), y::ivec2())
  , _bank(bank)
  , _brush(brush)
{
}

bool BrushPanel::event(const sf::Event& e)
{
  (void)e;
  return false;
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
      const TileBrush::Entry e =
          _brush.array[v[xx] + v[yy] * TileBrush::max_size];
      const Tileset& t = _bank.get_tileset(e.tileset);

      batch.add_sprite(t.get_texture(), Tileset::tile_size,
                       v * Tileset::tile_size, t.from_index(e.index));
    }
  }
  util.render_batch(batch);
}

TilePanel::TilePanel(const Databank& bank, TileBrush& brush)
  : Panel(y::ivec2(), y::ivec2())
  , _bank(bank)
  , _brush(brush)
  , _tileset_select(0)
  , _tile_hover{-1, -1}
{
}

bool TilePanel::event(const sf::Event& e)
{
  const Tileset& t = _bank.get_tileset(_tileset_select);

  // Update hover position.
  if (e.type == sf::Event::MouseMoved) {
    if (e.mouseMove.y < get_list_height()) {
      _tile_hover = {-1, -1};
    }
    else {
      _tile_hover = {e.mouseMove.x, e.mouseMove.y - get_list_height()};
      _tile_hover /= Tileset::tile_size;
    }
    return true;
  }
  if (e.type == sf::Event::MouseLeft) {
    _tile_hover = {-1, -1};
  }

  // Start drag.
  if (e.type == sf::Event::MouseButtonPressed &&
      e.mouseButton.button == sf::Mouse::Left) {
    start_drag(_tile_hover);
  }

  // End drag and update brush.
  if (e.type == sf::Event::MouseButtonReleased &&
      e.mouseButton.button == sf::Mouse::Left) {
    stop_drag();
    y::ivec2 min = y::min(get_drag_start(), _tile_hover);
    y::ivec2 max = y::max(get_drag_start(), _tile_hover);
    min = y::max(min, {0, 0});
    max = y::min(max, t.get_size() - y::ivec2{1, 1});
    max = y::min(max, min + y::ivec2{TileBrush::max_size - 1,
                                     TileBrush::max_size - 1});
    _brush.size = y::ivec2{1, 1} + max - min;
    for (y::int32 x = 0; x < _brush.size[xx]; ++x) {
      for (y::int32 y = 0; y < _brush.size[yy]; ++y) {
        auto& entry = _brush.array[x + y * TileBrush::max_size];
        entry.tileset = _tileset_select;
        entry.index = t.to_index(y::ivec2{x, y} + min);
      }
    }
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
    case sf::Keyboard::A:
      ++_tileset_select;
      break;
    default: {}
  }
  return false;
}

void TilePanel::update()
{
  y::roll<y::int32>(_tileset_select, 0, _bank.get_tilesets().size());

  const GlTexture& tex = _bank.get_tileset(_tileset_select).get_texture();
  set_size(tex.get_size() + y::ivec2{0, get_list_height()});
}

void TilePanel::draw(RenderUtil& util) const
{
  // Render panel.
  const Tileset& t = _bank.get_tileset(_tileset_select);
  const GlTexture& tex = t.get_texture();

  y::int32 tx_max = RenderUtil::to_grid(tex.get_size())[xx];
  y::int32 ty_max = y::min(y::size(7), _bank.get_tilesets().size());
  render_list_grid(util, c_panel, c_item, c_select,
                   _bank.get_tilesets(), _tileset_select,
                   y::ivec2(), {tx_max, ty_max});

  // Render tileset.
  util.render_fill(RenderUtil::from_grid({0, ty_max}),
                   tex.get_size(), c_dark_panel);
  util.render_sprite(tex, tex.get_size(),
                     RenderUtil::from_grid({0, ty_max}), y::ivec2());

  // Render hover.
  y::ivec2 start = is_dragging() ? get_drag_start() : _tile_hover;
  y::ivec2 min = y::min(start, _tile_hover);
  y::ivec2 max = y::max(start, _tile_hover);

  if (_tile_hover.in_region(y::ivec2(), t.get_size()) || is_dragging()) {
    min = y::max(min, {0, 0});
    max = y::min(max, t.get_size() - y::ivec2{1, 1});
    util.render_outline(
        min * Tileset::tile_size + y::ivec2{0, get_list_height()},
        (y::ivec2{1, 1} + (max - min)) * Tileset::tile_size, c_hover);
  }
}

y::int32 TilePanel::get_list_height() const
{
  return RenderUtil::from_grid()[yy] *
      y::min(y::size(7), _bank.get_tilesets().size());
}

LayerPanel::LayerPanel(const y::string& status)
  : Panel(y::ivec2(), y::ivec2())
  , _status(status)
  , _layer_select(0)
{
}

y::int8 LayerPanel::get_layer() const
{
  return _layer_select;
}

bool LayerPanel::event(const sf::Event& e)
{
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
  y::roll<y::int8>(_layer_select, -Cell::background_layers,
                   2 + Cell::foreground_layers);
  set_size(RenderUtil::from_grid({y::max(y::int32(_status.length()), 12), 5}));
}

void LayerPanel::draw(RenderUtil& util) const
{
  static y::string_vector layer_names{
    "1 Background",
    "2 Collision",
    "3 Foreground",
    "4 Actors"};

  render_list_grid(
      util, c_panel, c_item, c_select,
      layer_names, Cell::background_layers + _layer_select,
      {0, 0}, {y::max(y::int32(_status.length()), 12), 4});

  util.render_text_grid(_status, {0, 4}, c_item);
}

MapEditor::MapEditor(Databank& bank, RenderUtil& util, CellMap& map)
  : _bank(bank)
  , _util(util)
  , _map(map)
  , _hover{-1, -1}
  , _brush_panel(bank, _tile_brush)
  , _tile_panel(bank, _tile_brush)
  , _layer_panel(_layer_status)
{
  get_panel_ui().add(_brush_panel);
  get_panel_ui().add(_tile_panel);
  get_panel_ui().add(_layer_panel);
}

void MapEditor::event(const sf::Event& e)
{
  // Update hover position.
  if (e.type == sf::Event::MouseMoved) {
    _hover = {e.mouseMove.x, e.mouseMove.y};
  }
  if (e.type == sf::Event::MouseLeft) {
    _hover = {-1, -1};
  }

  // Start draw.
  if (e.type == sf::Event::MouseButtonPressed &&
      e.mouseButton.button == sf::Mouse::Left &&
      _layer_panel.get_layer() <= Cell::foreground_layers) {
    start_drag(_hover);
    _tile_edit_action = y::move_unique(
        new TileEditAction(_map, _layer_panel.get_layer()));
  }

  // End draw and commit tile edit.
  if (e.type == sf::Event::MouseButtonReleased &&
      e.mouseButton.button == sf::Mouse::Left) {
    stop_drag();
    get_undo_stack().new_action(y::move_unique(_tile_edit_action));
  }

  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  // Control camera.
  switch (e.key.code) {
    case sf::Keyboard::Escape:
      end();
      break;
    case sf::Keyboard::Right:
      _camera += {8, 0};
      break;
    case sf::Keyboard::Left:
      _camera += {-8, 0};
      break;
    case sf::Keyboard::Down:
      _camera += {0, 8};
      break;
    case sf::Keyboard::Up:
      _camera += {0, -8};
      break;
    default: {}
  }
}

void MapEditor::update()
{
  // Layout UI.
  const y::ivec2 spacing = RenderUtil::from_grid({0, 1});

  _tile_panel.set_origin(RenderUtil::from_grid());
  _brush_panel.set_origin(_tile_panel.get_origin() + spacing +
                          y::ivec2{0, _tile_panel.get_size()[yy]});
  _layer_panel.set_origin(_brush_panel.get_origin() + spacing +
                          y::ivec2{0, _brush_panel.get_size()[yy]});

  if (_hover[xx] < 0 || _hover[yy] < 0) {
    return;
  }

  // Update hover and status text.
  y::ivec2 world = camera_to_world(_hover);
  y::ivec2 tile = world.euclidean_div(Tileset::tile_size);
  _hover_cell = tile.euclidean_div(Cell::cell_size);
  _hover_tile = tile.euclidean_mod(Cell::cell_size);

  y::sstream ss;
  ss << _hover_cell[xx] << ", " << _hover_cell[yy];
  if (_map.is_coord_used(_hover_cell)) {
    ss << " : " << _bank.get_cell_name(*_map.get_coord(_hover_cell));
  }
  ss << " : " << _hover_tile[xx] << ", " << _hover_tile[yy];
  _layer_status = ss.str();

  // Continue tile drawing.
  if (!is_dragging()) {
    return;
  }
  y::ivec2 v;
  for (; v[xx] < _tile_brush.size[xx]; ++v[xx]) {
    for (v[yy] = 0; v[yy] < _tile_brush.size[yy]; ++v[yy]) {
      y::ivec2 c = (tile + v).euclidean_div(Cell::cell_size);
      y::ivec2 t = (tile + v).euclidean_mod(Cell::cell_size);
      if (!_map.is_coord_used(c)) {
        continue;
      }

      // TODO: not working.
      CellBlueprint* cell = _map.get_coord(c);
      const TileBrush::Entry& e =
          _tile_brush.array[v[xx] + v[yy] * TileBrush::max_size];
      Tile new_tile(&_bank.get_tileset(e.tileset), e.index);
      cell->set_tile(_tile_edit_action->layer, t, new_tile);
    }
  }
}

void MapEditor::draw() const
{
  _util.get_gl().bind_window(true);

  // Draw cells.
  RenderBatch batch;
  y::ivec2 c = _map.get_boundary_min();
  const y::ivec2& max = _map.get_boundary_max();

  for (y::int32 layer = -Cell::background_layers;
       layer <= Cell::foreground_layers; ++layer) {
    for (; c[xx] < max[xx]; ++c[xx]) {
      for (c[yy] = 0; c[yy] < max[yy]; ++c[yy]) {
        draw_cell_layer(batch, c, layer);

        _util.render_outline(
            world_to_camera(c * Tileset::tile_size * Cell::cell_size),
            Tileset::tile_size * Cell::cell_size, c_panel);
      }
    }
  }
  _util.render_batch(batch);

  // Draw cursor.
  if (_hover[xx] >= 0 && _hover[yy] >= 0 && _map.is_coord_used(_hover_cell)) {
    y::ivec2 t =
        (_hover_tile + _hover_cell * Cell::cell_size) * Tileset::tile_size;
    _util.render_outline(
        world_to_camera(t), Tileset::tile_size * _tile_brush.size, c_hover);
  }

  // Draw UI.
  get_panel_ui().draw(_util);
}

y::ivec2 MapEditor::world_to_camera(const y::ivec2& v) const
{
  const Resolution& r = _util.get_window().get_mode();
  return v - _camera + r.size / 2;
}

y::ivec2 MapEditor::camera_to_world(const y::ivec2& v) const
{
  const Resolution& r = _util.get_window().get_mode();
  return v + _camera - r.size / 2;
}

void MapEditor::draw_cell_layer(
    RenderBatch& batch, const y::ivec2& coord, y::int8 layer) const
{
  if (!_map.is_coord_used(coord)) {
    return;
  }
  const CellBlueprint* cell = _map.get_coord(coord);

  y::ivec2 v;
  for (; v[xx] < Cell::cell_size[xx]; ++v[xx]) {
    for (v[yy] = 0; v[yy] < Cell::cell_size[yy]; ++v[yy]) {
      const Tile& t = cell->get_tile(layer, v);
      if (!t.tileset) {
        continue;
      }

      y::ivec2 camera = world_to_camera(
          Tileset::tile_size * (v + coord * Cell::cell_size));

      batch.add_sprite(t.tileset->get_texture(), Tileset::tile_size,
                       camera, t.tileset->from_index(t.index));
    }
  }
}
