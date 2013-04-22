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
const Colour c_select(.9f, .9f, .9f, 1.f);

// TODO: take this out into a nice happy class.
void render_list_grid(
    RenderUtil& util,
    const Colour& panel, const y::vector<Colour>& items,
    const y::string_vector& source, y::size select,
    const y::ivec2& origin, const y::ivec2& size)
{
  util.render_colour_grid(origin, size, panel);

  y::int32 w = size[xx];
  y::int32 h = size[yy];
  y::int32 offset = std::max<y::int32>(0,
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

TilePanel::TilePanel(const Databank& bank)
  : Panel({RenderUtil::font_width,
           RenderUtil::font_height}, y::ivec2())
  , _bank(bank)
  , _tileset_select(0)
{
}

bool TilePanel::event(const sf::Event& e)
{
  if (e.type == sf::Event::MouseWheelMoved) {
    _tileset_select -= e.mouseWheel.delta;
    return true;
  }
  if (e.type != sf::Event::KeyPressed) {
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
  y::int32 y_max = tex.get_size()[yy] +
    std::min(y::size(7), _bank.get_tilesets().size()) * RenderUtil::font_height;

  set_size({tex.get_size()[xx], y_max});
}

void TilePanel::draw(RenderUtil& util) const
{
  // Render panel.
  const Tileset& t = _bank.get_tileset(_tileset_select);
  const GlTexture& tex = t.get_texture();

  y::int32 tx_max = tex.get_size()[xx] / RenderUtil::font_width;
  y::int32 ty_max = std::min(y::size(7), _bank.get_tilesets().size());
  render_list_grid(util, c_panel, c_item, c_select,
                   _bank.get_tilesets(), _tileset_select,
                   y::ivec2(), {tx_max, ty_max});

  // Render tileset.
  util.render_colour({0, ty_max * RenderUtil::font_height},
                     tex.get_size(), c_dark_panel);
  util.render_sprite(tex, tex.get_size(),
                     {0, ty_max * RenderUtil::font_height}, y::ivec2());
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
  set_size({12 * RenderUtil::font_width,
            3 * RenderUtil::font_width});
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
  , _tile_panel(bank)
{
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
  _layer_panel.set_origin(_tile_panel.get_origin() +
                          y::ivec2{0, _tile_panel.get_size()[yy]});
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

      const y::ivec2& size = t.tileset->get_size();
      batch.add_sprite(
          t.tileset->get_texture(), {Tileset::tile_width, Tileset::tile_height},
          camera, {y::int32(t.index) % size[xx], y::int32(t.index) / size[yy]});
    }
  }
}
