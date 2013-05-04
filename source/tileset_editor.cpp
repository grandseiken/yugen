#include "tileset_editor.h"

#include "databank.h"
#include "render_util.h"
#include "tileset.h"

#include <SFML/Window.hpp>

TilesetEditor::TilesetEditor(Databank& bank, RenderUtil& util, Tileset& tileset)
  : _bank(bank)
  , _util(util)
  , _tileset(tileset)
{
}

void TilesetEditor::event(const sf::Event& e)
{ 
  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  switch (e.key.code) {
    case sf::Keyboard::Escape:
      end();
      break;
    default: {}
  }
}

void TilesetEditor::update()
{
}

void TilesetEditor::draw() const
{
  _util.get_gl().bind_window(true, true);
}
