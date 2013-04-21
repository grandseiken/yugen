#include "map_editor.h"

#include "cell.h"
#include "databank.h"
#include "render_util.h"
#include "tileset.h"
#include "window.h"

#include <SFML/Window.hpp>

MapEditor::MapEditor(Databank& bank, RenderUtil& util, CellMap& map)
  : _bank(bank), _util(util), _map(map)
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

void MapEditor::draw() const
{
  _util.get_gl().bind_window(true);

  RenderBatch batch;
  CellCoord c = _map.get_boundary_min();
  const CellCoord& max = _map.get_boundary_max();

  for (; c.x < max.x; ++c.x) {
    for (; c.y < max.y; ++c.y) {
      draw_cell_layer(batch, c, 0);
    }
  }

  _util.render_batch(batch);

  Colour window(.1f, .1f, .1f, .7f);
  Colour text(.8f, .8f, .8f, 1.f);

  _util.render_colour_grid(1, 1, 32, 32, window);
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
          camera[x], camera[y],
          t.index % t.tileset->get_width(),
          t.index / t.tileset->get_width());
    }
  }
}
