#include "map_editor.h"

#include "cell.h"
#include "databank.h"
#include "render_util.h"
#include "tileset.h"
#include "window.h"

#include <SFML/Window.hpp>

MapEditor::MapEditor(Databank& bank, RenderUtil& util, CellMap& map)
  : _bank(bank)
  , _util(util)
  , _map(map)
  , _tileset_select(0)
  , _layer_select(0)
{
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
    case sf::Keyboard::Q:
      --_tileset_select;
      break;
    case sf::Keyboard::A:
      ++_tileset_select;
      break;
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
    default: {}
  }
}

void MapEditor::update()
{
  y::roll<y::int32>(_tileset_select, 0, _bank.get_tilesets().size());
  y::roll<y::int8>(_layer_select, -(signed)Cell::background_layers,
                   1 + Cell::foreground_layers);
}

// TODO: take this out into a nice happy class.
void render_list_grid(
    RenderUtil& util,
    const Colour& panel, const y::vector<Colour>& items,
    const y::string_vector& source, y::size select,
    y::int32 x, y::int32 y, y::int32 w, y::int32 h)
{
  util.render_colour_grid(x, y, w, h, panel);

  y::int32 offset = std::max<y::int32>(0,
    select >= source.size() - h / 2 - 1 ? source.size() - h : select - h / 2);

  for (y::int32 i = 0; i < h; ++i) {
    if (i + offset >= signed(items.size()) ||
        i + offset >= signed(source.size())) {
      break;
    }

    y::string s = source[i + offset];
    s = signed(s.length()) > w ? s.substr(0, w - 3) + "..." : s;
    util.render_text_grid(s, x, y + i, items[i + offset]);
  }
}

// TODO: likewise.
void render_list_grid(
    RenderUtil& util,
    const Colour& panel, const Colour& item, const Colour& s_item,
    const y::string_vector& source, y::size select,
    y::int32 x, y::int32 y, y::int32 w, y::int32 h)
{
  y::vector<Colour> items;
  for (y::size n = 0; n < source.size(); ++n) {
    items.push_back(n == select ? s_item : item);
  }

  render_list_grid(util, panel, items, source, select, x, y, w, h);
}

void MapEditor::draw() const
{
  _util.get_gl().bind_window(true);

  RenderBatch batch;
  CellCoord c = _map.get_boundary_min();
  const CellCoord& max = _map.get_boundary_max();

  for (y::int32 layer = -signed(Cell::background_layers);
       layer <= signed(Cell::foreground_layers); ++layer) {
    for (; c.x < max.x; ++c.x) {
      for (; c.y < max.y; ++c.y) {
        draw_cell_layer(batch, c, layer);
      }
    }
  }

  _util.render_batch(batch);

  Colour panel(.2f, .2f, .2f, .7f);
  Colour dark_panel(0.f, 0.f, 0.f, .7f);
  Colour item(.6f, .6f, .6f, 1.f);
  Colour select(.9f, .9f, .9f, 1.f);
  const Resolution& mode = _util.get_window().get_mode();

  // Render tileset window.
  const Tileset& t = _bank.get_tileset(_tileset_select);
  y::size ty_max = 7;
  y::size tx_max = t.get_texture().get_width() / RenderUtil::font_width;
  ty_max = std::min(ty_max, _bank.get_tilesets().size());
  render_list_grid(
      _util, panel, item, select,
      _bank.get_tilesets(), _tileset_select, 1, 1, tx_max, ty_max);

  // Render tileset.
  _util.render_colour(
      RenderUtil::font_width, (1 + ty_max) * RenderUtil::font_width,
      t.get_texture().get_width(), t.get_texture().get_height(), dark_panel);
  _util.render_sprite(
      t.get_texture(),
      t.get_texture().get_width(), t.get_texture().get_height(),
      RenderUtil::font_width, (1 + ty_max) * RenderUtil::font_width, 0, 0);

  // Render layer indicator.
  static y::string_vector layer_names{"Background", "Collision", "Foreground"};
  render_list_grid(
      _util, panel, item, select,
      layer_names, Cell::background_layers + _layer_select,
      mode.width / RenderUtil::font_width - 11, 1, 10, 3);
}

y::ivec2 MapEditor::world_to_camera(const y::ivec2& v) const
{
  const Resolution& r = _util.get_window().get_mode();
  return v - _camera + y::ivec2(signed(r.width) / 2, signed(r.height) / 2);
}

y::ivec2 MapEditor::camera_to_world(const y::ivec2& v) const
{
  const Resolution& r = _util.get_window().get_mode();
  return v + _camera - y::ivec2(signed(r.width) / 2, signed(r.height) / 2);
}

void MapEditor::draw_cell_layer(
    RenderBatch& batch, const CellCoord& coord, y::int8 layer) const
{
  if (!_map.is_coord_used(coord)) {
    return;
  }
  const CellBlueprint* cell = _map.get_coord(coord);

  for (y::size x = 0; x < Cell::cell_width; ++x) {
    for (y::size y = 0; y < Cell::cell_height; ++y) {
      const Tile& t = cell->get_tile(layer, x, y);
      if (!t.tileset) {
        continue;
      }

      y::ivec2 camera = world_to_camera(y::ivec2(
          signed(Tileset::tile_width) * (
              signed(x) + coord.x * signed(Cell::cell_width)),
          signed(Tileset::tile_height) * (
              signed(y) + coord.y * signed(Cell::cell_height))));

      batch.add_sprite(
          t.tileset->get_texture(), Tileset::tile_width, Tileset::tile_height,
          camera[xx], camera[yy],
          t.index % t.tileset->get_width(),
          t.index / t.tileset->get_width());
    }
  }
}
