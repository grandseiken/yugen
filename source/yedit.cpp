#include "yedit.h"

#include "databank.h"
#include "gl_util.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "tileset.h"
#include "window.h"

#include <SFML/Window.hpp>

Yedit::Yedit(Databank& bank, Window& window, GlUtil& gl, RenderUtil& util)
  : _bank(bank)
  , _window(window)
  , _gl(gl)
  , _util(util)
{
}

void Yedit::event(const sf::Event& e)
{
  if (e.type == sf::Event::Closed ||
      (e.type == sf::Event::KeyPressed &&
       e.key.code == sf::Keyboard::Escape)) {
    end();
  }
}

void Yedit::update()
{
}

void Yedit::draw() const
{
  _gl.bind_window();
  const Resolution& screen = _window.get_mode();
  _util.set_resolution(screen.width, screen.height);

  RenderBatch batch;
  const Tileset& t = _bank.get_tileset(_bank.get_tileset_list()[0]);
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
  Colour grey(.5f, .5f, .5f);

  y::size i = 1;
  _util.render_text_grid("Tilesets", 1, i++, grey);
  for (const auto& s :_bank.get_tileset_list()) {
    _util.render_text_grid(s, 2, i++, white);
  }

  i++;
  _util.render_text_grid("Cells", 1, i++, grey);
  for (const auto& s :_bank.get_cell_list()) {
    _util.render_text_grid(s, 2, i++, white);
  }

  i++;
  _util.render_text_grid("Maps", 1, i++, grey);
  for (const auto& s :_bank.get_map_list()) {
    _util.render_text_grid(s, 2, i++, white);
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
  stack.push(y::move_unique(new Yedit(databank, window, gl, util)));
  stack.run(window);
  return 0;
}
