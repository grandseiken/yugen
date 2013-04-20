#include "yedit.h"

#include "databank.h"
#include "gl_util.h"
#include "map_editor.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "tileset.h"
#include "window.h"

#include <SFML/Window.hpp>

Yedit::Yedit(Databank& bank, RenderUtil& util)
  : _bank(bank)
  , _util(util)
  , _map_select(0)
{
}

void Yedit::event(const sf::Event& e)
{
  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  switch (e.key.code) {
    case sf::Keyboard::Down:
      ++_map_select;
      break;
    case sf::Keyboard::Up:
      --_map_select;
      break;
    case sf::Keyboard::Return:
      push(y::move_unique(new MapEditor(
          _bank, _util, _bank.get_map(_map_select))));
      break;
    case sf::Keyboard::Escape:
      end();
      break;
    default: {}
  }
}

void Yedit::update()
{
  _map_select = y::clamp<y::size>(_map_select, 0, _bank.get_maps().size());
}

void Yedit::draw() const
{
  _util.get_gl().bind_window(true);
  const Resolution& screen = _util.get_window().get_mode();
  _util.set_resolution(screen.width, screen.height);

  _util.render_colour(256 - 8, 0, 16 + 32 * 4, 16 + 32, 1.f, 1.f, 1.f, .5f);
  RenderBatch batch;
  const Tileset& t = _bank.get_tileset(_bank.get_tilesets()[0]);
  batch.add_sprite(t.get_texture(), Tileset::tile_width, Tileset::tile_height,
                   256, 8, 4, 0);
  batch.add_sprite(t.get_texture(), Tileset::tile_width, Tileset::tile_height,
                   256, 40, 5, 0);
  batch.add_sprite(t.get_texture(), Tileset::tile_width, Tileset::tile_height,
                   256, 72, 6, 0);
  batch.add_sprite(t.get_texture(), Tileset::tile_width, Tileset::tile_height,
                   256, 104, 7, 0);
  _util.render_batch(batch);

  Colour white(1.f, 1.f, 1.f);
  Colour red(1.f, 0.f, 0.f);
  Colour grey(.5f, .5f, .5f);

  y::size i = 1;
  _util.render_text_grid("Tilesets", 1, i++, grey);
  for (const auto& s :_bank.get_tilesets()) {
    _util.render_text_grid(s, 2, i++, white);
  }

  i++;
  _util.render_text_grid("Cells", 1, i++, grey);
  for (const auto& s :_bank.get_cells()) {
    _util.render_text_grid(s, 2, i++, white);
  }

  i++;
  y::size n = 0;
  _util.render_text_grid("Maps", 1, i++, grey);
  for (const auto& s :_bank.get_maps()) {
    _util.render_text_grid(s, 2, i++, n++ == _map_select ? red : white);
  }
}

int main(int argc, char** argv)
{
  Window window("Crunk Yedit", 24, 0, 0, false, true);
  PhysicalFilesystem filesystem("data");
  GlUtil gl(filesystem, window);
  if (!gl) {
    return 1;
  }
  Databank databank(filesystem, gl);
  RenderUtil util(gl);

  ModalStack stack;
  stack.push(y::move_unique(new Yedit(databank, util)));
  stack.run(window);
  return 0;
}
