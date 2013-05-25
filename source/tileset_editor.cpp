#include "tileset_editor.h"

#include "databank.h"
#include "render_util.h"
#include "tileset.h"
#include "window.h"

#include <SFML/Window.hpp>

void render_collision(RenderUtil& util, const y::ivec2& pixels,
                      y::int32 collision, const y::fvec4& colour)
{
  static const y::ivec2& t = Tileset::tile_size;
  const y::ivec2 ul = pixels;
  const y::ivec2 ur = pixels + y::ivec2{t[xx], 0};
  const y::ivec2 dl = pixels + y::ivec2{0, t[yy]};
  const y::ivec2 dr = pixels + t;
  const y::ivec2 u = pixels + y::ivec2{t[xx] / 2, 0};
  const y::ivec2 d = pixels + y::ivec2{t[xx] / 2, t[yy]};
  const y::ivec2 l = pixels + y::ivec2{0, t[yy] / 2};
  const y::ivec2 r = pixels + y::ivec2{t[xx], t[yy] / 2};

  switch (collision) {
    case Tileset::COLLIDE_FULL:
      util.render_fill(pixels, t, colour);
      break;
    case Tileset::COLLIDE_HALF_U:
      util.render_fill(pixels, t / y::ivec2{1, 2}, colour);
      break;
    case Tileset::COLLIDE_HALF_D:
      util.render_fill(pixels + y::ivec2{0, t[yy] / 2},
                       t / y::ivec2{1, 2}, colour);
      break;
    case Tileset::COLLIDE_HALF_L:
      util.render_fill(pixels, t / y::ivec2{2, 1}, colour);
      break;
    case Tileset::COLLIDE_HALF_R:
      util.render_fill(pixels + y::ivec2{t[xx] / 2, 0},
                       t / y::ivec2{2, 1}, colour);
      break;
    case Tileset::COLLIDE_SLOPE1_UL:
      util.render_fill(dl, ul, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPE1_UR:
      util.render_fill(dl, ur, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPE1_DL:
      util.render_fill(ul, dl, ur, colour);
      break;
    case Tileset::COLLIDE_SLOPE1_DR:
      util.render_fill(ul, dr, ur, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_UL_A:
      util.render_fill(d, ul, dl, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_UR_A:
      util.render_fill(d, ur, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_DL_A:
      util.render_fill(u, ul, dl, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_DR_A:
      util.render_fill(u, ur, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_UL_A:
      util.render_fill(l, dl, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_UR_A:
      util.render_fill(r, dl, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_DL_A:
      util.render_fill(l, ul, ur, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_DR_A:
      util.render_fill(r, ul, ur, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_UL_B:
      util.render_fill(pixels, t / y::ivec2{2, 1}, colour);
      util.render_fill(dr, u, d, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_UR_B:
      util.render_fill(pixels + y::ivec2{t[xx] / 2, 0},
                       t / y::ivec2{2, 1}, colour);
      util.render_fill(dl, u, d, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_DL_B:
      util.render_fill(pixels, t / y::ivec2{2, 1}, colour);
      util.render_fill(ur, u, d, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_DR_B:
      util.render_fill(pixels + y::ivec2{t[xx] / 2, 0},
                       t / y::ivec2{2, 1}, colour);
      util.render_fill(ul, u, d, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_UL_B:
      util.render_fill(pixels + y::ivec2{0, t[yy] / 2},
                       t / y::ivec2{1, 2}, colour);
      util.render_fill(ul, l, r, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_UR_B:
      util.render_fill(pixels + y::ivec2{0, t[yy] / 2},
                       t / y::ivec2{1, 2}, colour);
      util.render_fill(ur, l, r, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_DL_B:
      util.render_fill(pixels, t / y::ivec2{1, 2}, colour);
      util.render_fill(dl, l, r, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_DR_B:
      util.render_fill(pixels, t / y::ivec2{1, 2}, colour);
      util.render_fill(dr, l, r, colour);
      break;
    default: {}
  }
}

SetCollideAction::SetCollideAction(Tileset& tileset, const y::ivec2& coord,
                                   y::int32 old_collide, y::int32 new_collide)
  : tileset(tileset)
  , coord(coord)
  , old_collide(old_collide)
  , new_collide(new_collide)
{
}

void SetCollideAction::redo() const
{
  tileset.set_collision(coord, Tileset::Collision(new_collide));
}

void SetCollideAction::undo() const
{
  tileset.set_collision(coord, Tileset::Collision(old_collide));
}

TilesetPanel::TilesetPanel(Tileset& tileset, y::int32& collide_select,
                           UndoStack& undo_stack)
  : Panel(y::ivec2(), y::ivec2(), 0)
  , _tileset(tileset)
  , _collide_select(collide_select)
  , _undo_stack(undo_stack)
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
    if (e.mouseButton.button == sf::Mouse::Left &&
        _tileset.get_collision(_hover) != _collide_select) {
      _undo_stack.new_action(y::move_unique(new SetCollideAction(
          _tileset, _hover, _tileset.get_collision(_hover),
          _collide_select)));
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
                     0.f, colour::white);

  if (_hover >= y::ivec2()) {
    util.render_outline(_hover * Tileset::tile_size,
                        Tileset::tile_size, colour::hover);
  }

  if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
    return;
  }
  for (auto it = y::cartesian(_tileset.get_size()); it; ++it) {
    render_collision(util, *it * Tileset::tile_size,
                     _tileset.get_collision(*it), colour::highlight);
  }
}

const y::int32 CollidePanel::entries_per_row = 8;

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

  if (e.type == sf::Event::MouseButtonPressed && _hover >= y::ivec2()) {
    y::ivec2 v = _hover.euclidean_div(Tileset::tile_size);
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
      y::min(entries_per_row, y::int32(Tileset::COLLIDE_SIZE)),
      1 + (Tileset::COLLIDE_SIZE - 1) / entries_per_row});
}

void CollidePanel::draw(RenderUtil& util) const
{
  for (y::int32 i = 0; i < Tileset::COLLIDE_SIZE; ++i) {
     y::ivec2 v{i % entries_per_row, i / entries_per_row};
     render_collision(util, Tileset::tile_size * v, i, colour::hover);
     if (i == _collide_select) {
       util.render_outline(Tileset::tile_size * v, Tileset::tile_size,
                           colour::select);
     }
     else if (_hover >= y::ivec2() &&
              _hover.euclidean_div(Tileset::tile_size) == v) {
       util.render_outline(Tileset::tile_size * v, Tileset::tile_size,
                           colour::hover);
     }
  }
}

TilesetEditor::TilesetEditor(Databank& bank, RenderUtil& util, Tileset& tileset)
  : _bank(bank)
  , _util(util)
  , _tileset(tileset)
  , _collide_select(0)
  , _tileset_panel(tileset, _collide_select, get_undo_stack())
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
