#include "tileset_editor.h"

#include "databank.h"
#include "render_util.h"
#include "tileset.h"
#include "window.h"

#include <SFML/Window.hpp>

void render_collision(RenderUtil& util, const y::ivec2& pixels,
                      y::int32 collision, const Colour& colour)
{
  if (collision == Tileset::COLLIDE_NONE) {
    return;
  }
  if (collision == Tileset::COLLIDE_FULL) {
    util.render_fill(pixels, Tileset::tile_size, colour);
  }
}

TilesetPanel::TilesetPanel(Tileset& tileset, y::int32& collide_select)
  : Panel(y::ivec2(), y::ivec2(), 0)
  , _tileset(tileset)
  , _collide_select(collide_select)
  , _hover{-1, -1}
{
}

bool TilesetPanel::event(const sf::Event& e)
{
  if (e.type == sf::Event::MouseMoved) {
    _hover = y::ivec2{e.mouseMove.x, e.mouseMove.y} / Tileset::tile_size;
    return true;
  }

  if (e.type == sf::Event::MouseLeft) {
    _hover = {-1, -1};
  }

  if (e.type == sf::Event::MouseButtonPressed) {
    if (e.mouseButton.button == sf::Mouse::Left) {
      _tileset.set_collision(_hover, Tileset::Collision(_collide_select));
    }
    else if (e.mouseButton.button == sf::Mouse::Right) {
      _collide_select = _tileset.get_collision(_hover);
    }
  }

  return false;
}

void TilesetPanel::update()
{
  set_size(_tileset.get_texture().get_size());
}

void TilesetPanel::draw(RenderUtil& util) const
{
  const GlTexture& texture = _tileset.get_texture();
  util.render_sprite(texture, texture.get_size(), y::ivec2(), y::ivec2(),
                     0.f, Colour::white);

  if (_hover > y::ivec2()) {
    util.render_outline(_hover * Tileset::tile_size,
                        Tileset::tile_size, Colour::hover);
  }

  if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
    return;
  }
  for (auto it = y::cartesian(_tileset.get_size()); it; ++it) {
    render_collision(util, *it * Tileset::tile_size,
                     _tileset.get_collision(*it), Colour::highlight);
  }
}

CollidePanel::CollidePanel(y::int32& collide_select)
  : Panel(y::ivec2(), y::ivec2(), 0)
  , _collide_select(collide_select)
  , _hover{-1, -1}
{
}

bool CollidePanel::event(const sf::Event& e)
{
  if (e.type == sf::Event::MouseMoved) {
    _hover = {e.mouseMove.x, e.mouseMove.y};
    return true;
  }

  if (e.type == sf::Event::MouseLeft) {
    _hover = {-1, -1};
  }

  if (e.type == sf::Event::MouseWheelMoved) {
    _collide_select -= e.mouseWheel.delta;
    return true;
  }

  if (e.type == sf::Event::MouseButtonPressed && _hover > y::ivec2()) {
    y::ivec2 v = _hover / Tileset::tile_size;
    y::int32 i = v[xx] + v[yy] * entries_per_row;
    if (i < Tileset::COLLIDE_SIZE) {
      _collide_select = i;
    }
    return true;
  }

  if (e.type != sf::Event::KeyPressed) {
    return false;
  }

  switch (e.key.code) {
    case sf::Keyboard::Q:
      --_collide_select;
      break;
    case sf::Keyboard::E:
      ++_collide_select;
      break;
    default: {}
  }

  return false;
}

void CollidePanel::update()
{
  set_size(Tileset::tile_size * y::ivec2{
      entries_per_row, 1 + (Tileset::COLLIDE_SIZE - 1) / entries_per_row});
}

void CollidePanel::draw(RenderUtil& util) const
{
  for (y::int32 i = 0; i < Tileset::COLLIDE_SIZE; ++i) {
     y::ivec2 v{i % Tileset::COLLIDE_SIZE, i / Tileset::COLLIDE_SIZE};
     render_collision(util, Tileset::tile_size * v, i, Colour::hover);
     if (i == _collide_select) {
       util.render_outline(Tileset::tile_size * v, Tileset::tile_size,
                           Colour::select);
     }
     else if (_hover > y::ivec2() && _hover / Tileset::tile_size == v) {
       util.render_outline(Tileset::tile_size * v, Tileset::tile_size,
                           Colour::hover);
     }
  }
}

TilesetEditor::TilesetEditor(Databank& bank, RenderUtil& util, Tileset& tileset)
  : _bank(bank)
  , _util(util)
  , _tileset(tileset)
  , _collide_select(0)
  , _tileset_panel(tileset, _collide_select)
  , _collide_panel(_collide_select)
{
  get_panel_ui().add(_tileset_panel);
  get_panel_ui().add(_collide_panel);
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
  y::roll<y::int32>(_collide_select, 0, Tileset::COLLIDE_SIZE);

  y::ivec2 spacer = RenderUtil::from_grid();
  const Resolution& r = _util.get_window().get_mode();

  const y::int32 total_width =
      (_collide_panel.get_size() + _tileset_panel.get_size() + spacer)[xx];

  _tileset_panel.set_origin(
      (r.size - y::ivec2{total_width, _tileset_panel.get_size()[yy]}) / 2);
  _collide_panel.set_origin({
      (_tileset_panel.get_origin() + _tileset_panel.get_size() + spacer)[xx],
      (r.size - _collide_panel.get_size())[yy] / 2});
}

void TilesetEditor::draw() const
{
  get_panel_ui().draw(_util);
}