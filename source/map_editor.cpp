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
  : size{0, 0}
  , array(new Entry[max_size * max_size])
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
  set_size({tiles[xx] * Tileset::tile_width,
            tiles[yy] * Tileset::tile_height});
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

      y::ivec2 pos = {v[xx] * Tileset::tile_width,
                      v[yy] * Tileset::tile_height};
      batch.add_sprite(t.get_texture(),
                       {Tileset::tile_width, Tileset::tile_height},
                       pos, t.from_index(e.index));
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
      _tile_hover = {
        e.mouseMove.x / Tileset::tile_width,
        (e.mouseMove.y - get_list_height()) / Tileset::tile_height};
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

  if ((_tile_hover[xx] >= 0 && _tile_hover[yy] >= 0 &&
      _tile_hover[xx] < t.get_size()[xx] &&
      _tile_hover[yy] < t.get_size()[yy]) || is_dragging()) {
    min = y::max(min, {0, 0});
    max = y::min(max, t.get_size() - y::ivec2{1, 1});
    util.render_outline(
        {min[xx] * Tileset::tile_width,
         min[yy] * Tileset::tile_height + get_list_height()},
        {(1 + (max - min)[xx]) * Tileset::tile_width,
         (1 + (max - min)[yy]) * Tileset::tile_height}, c_hover);
  }
}

y::int32 TilePanel::get_list_height() const
{
  return RenderUtil::from_grid()[yy] *
      y::min(y::size(7), _bank.get_tilesets().size());
}

LayerPanel::LayerPanel()
  : Panel(y::ivec2(), y::ivec2())
  , _layer_select(0)
{
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
                   1 + Cell::foreground_layers);
  set_size(RenderUtil::from_grid({12, 3}));
}

void LayerPanel::draw(RenderUtil& util) const
{
  static y::string_vector layer_names{
    "1 Background",
    "2 Collision",
    "3 Foreground"};

  render_list_grid(
      util, c_panel, c_item, c_select,
      layer_names, Cell::background_layers + _layer_select,
      {0, 0}, {12, 3});
}

MapEditor::MapEditor(Databank& bank, RenderUtil& util, CellMap& map)
  : _bank(bank)
  , _util(util)
  , _map(map)
  , _brush_panel(bank, _tile_brush)
  , _tile_panel(bank, _tile_brush)
{
  get_panel_ui().add(_brush_panel);
  get_panel_ui().add(_tile_panel);
  get_panel_ui().add(_layer_panel);
}

void MapEditor::event(const sf::Event& e)
{
  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  switch (e.key.code) {
    case sf::Keyboard::Escape:
      end();
      break;
    case sf::Keyboard::Right:
      _camera += {1, 0};
      break;
    case sf::Keyboard::Left:
      _camera += {-1, 0};
      break;
    case sf::Keyboard::Down:
      _camera += {0, 1};
      break;
    case sf::Keyboard::Up:
      _camera += {0, -1};
      break;
    default: {}
  }
}

void MapEditor::update()
{
  y::ivec2 spacing = RenderUtil::from_grid({0, 1});

  _tile_panel.set_origin(RenderUtil::from_grid());
  _brush_panel.set_origin(_tile_panel.get_origin() + spacing +
                          y::ivec2{0, _tile_panel.get_size()[yy]});
  _layer_panel.set_origin(_brush_panel.get_origin() + spacing +
                          y::ivec2{0, _brush_panel.get_size()[yy]});
}

void MapEditor::draw() const
{
  _util.get_gl().bind_window(true);

  RenderBatch batch;
  CellCoord c = _map.get_boundary_min();
  const CellCoord& max = _map.get_boundary_max();

  for (y::int32 layer = -Cell::background_layers;
       layer <= Cell::foreground_layers; ++layer) {
    for (; c.x < max.x; ++c.x) {
      for (; c.y < max.y; ++c.y) {
        draw_cell_layer(batch, c, layer);
      }
    }
  }

  _util.render_batch(batch);
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
    RenderBatch& batch, const CellCoord& coord, y::int8 layer) const
{
  if (!_map.is_coord_used(coord)) {
    return;
  }
  const CellBlueprint* cell = _map.get_coord(coord);

  for (y::int32 x = 0; x < Cell::cell_width; ++x) {
    for (y::int32 y = 0; y < Cell::cell_height; ++y) {
      const Tile& t = cell->get_tile(layer, x, y);
      if (!t.tileset) {
        continue;
      }

      y::ivec2 camera = world_to_camera(y::ivec2(
          Tileset::tile_width * (x + coord.x * Cell::cell_width),
          Tileset::tile_height * (y + coord.y * Cell::cell_height)));

      batch.add_sprite(t.tileset->get_texture(),
                       {Tileset::tile_width, Tileset::tile_height},
                       camera, t.tileset->from_index(t.index));
    }
  }
}
