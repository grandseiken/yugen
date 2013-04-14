#include "databank.h"
#include "gl_util.h"
#include "modal.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "window.h"

#include <SFML/Window.hpp>

class Editor : public Modal, public y::no_copy {
public:

  Editor(Databank& bank, Window& window, GlUtil& gl, RenderUtil& util);
  virtual ~Editor() {}

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  Databank& _bank;
  Window& _window;
  GlUtil& _gl;
  RenderUtil& _util;

};

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
  stack.push(y::move_unique(new Editor(databank, window, gl, util)));
  stack.run(window);
  return 0;
}

Editor::Editor(Databank& bank, Window& window, GlUtil& gl, RenderUtil& util)
  : _bank(bank)
  , _window(window)
  , _gl(gl)
  , _util(util)
{
}

void Editor::event(const sf::Event& e)
{
  if (e.type == sf::Event::Closed ||
      (e.type == sf::Event::KeyPressed &&
       e.key.code == sf::Keyboard::Escape)) {
    end();
  }
}

void Editor::update()
{
}

void Editor::draw() const
{
  _gl.bind_window();
  const Resolution& screen = _window.get_mode();
  _util.set_resolution(screen.width, screen.height);

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
