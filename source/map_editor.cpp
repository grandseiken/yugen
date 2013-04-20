#include "map_editor.h"

#include "render_util.h"

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
    default: {}
  }
}

void MapEditor::update()
{
}

void MapEditor::draw() const
{
  _util.get_gl().bind_window(true);
}
