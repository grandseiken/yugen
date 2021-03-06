#include "tile_edit.h"

#include "../data/bank.h"
#include "../data/tileset.h"
#include "../render/util.h"
#include "../render/window.h"
#include "../../gen/proto/cell.pb.h"

#include <SFML/Window.hpp>

namespace {

void render_collision(RenderUtil& util, const y::ivec2& pixels,
                      std::int32_t collision, const y::fvec4& colour)
{
  static const y::ivec2& t = Tileset::tile_size;
  const y::ivec2 ul = pixels + Tileset::ul;
  const y::ivec2 ur = pixels + Tileset::ur;
  const y::ivec2 dl = pixels + Tileset::dl;
  const y::ivec2 dr = pixels + Tileset::dr;
  const y::ivec2 u = pixels + Tileset::u;
  const y::ivec2 d = pixels + Tileset::d;
  const y::ivec2 l = pixels + Tileset::l;
  const y::ivec2 r = pixels + Tileset::r;

  switch (collision) {
    case Tileset::COLLIDE_FULL:
      util.irender_fill(pixels, t, colour);
      break;
    case Tileset::COLLIDE_HALF_U:
      util.irender_fill(pixels, t / y::ivec2{1, 2}, colour);
      break;
    case Tileset::COLLIDE_HALF_D:
      util.irender_fill(pixels + y::ivec2{0, t[yy] / 2},
                        t / y::ivec2{1, 2}, colour);
      break;
    case Tileset::COLLIDE_HALF_L:
      util.irender_fill(pixels, t / y::ivec2{2, 1}, colour);
      break;
    case Tileset::COLLIDE_HALF_R:
      util.irender_fill(pixels + y::ivec2{t[xx] / 2, 0},
                        t / y::ivec2{2, 1}, colour);
      break;
    case Tileset::COLLIDE_SLOPE1_UL:
      util.irender_fill(dl, ul, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPE1_UR:
      util.irender_fill(dl, ur, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPE1_DL:
      util.irender_fill(ul, dl, ur, colour);
      break;
    case Tileset::COLLIDE_SLOPE1_DR:
      util.irender_fill(ul, dr, ur, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_UL_A:
      util.irender_fill(d, ul, dl, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_UR_A:
      util.irender_fill(d, ur, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_DL_A:
      util.irender_fill(u, ul, dl, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_DR_A:
      util.irender_fill(u, ur, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_UL_A:
      util.irender_fill(l, dl, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_UR_A:
      util.irender_fill(r, dl, dr, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_DL_A:
      util.irender_fill(l, ul, ur, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_DR_A:
      util.irender_fill(r, ul, ur, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_UL_B:
      util.irender_fill(pixels, t / y::ivec2{2, 1}, colour);
      util.irender_fill(dr, u, d, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_UR_B:
      util.irender_fill(pixels + y::ivec2{t[xx] / 2, 0},
                        t / y::ivec2{2, 1}, colour);
      util.irender_fill(dl, u, d, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_DL_B:
      util.irender_fill(pixels, t / y::ivec2{2, 1}, colour);
      util.irender_fill(ur, u, d, colour);
      break;
    case Tileset::COLLIDE_SLOPE2_DR_B:
      util.irender_fill(pixels + y::ivec2{t[xx] / 2, 0},
                        t / y::ivec2{2, 1}, colour);
      util.irender_fill(ul, u, d, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_UL_B:
      util.irender_fill(pixels + y::ivec2{0, t[yy] / 2},
                        t / y::ivec2{1, 2}, colour);
      util.irender_fill(ul, l, r, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_UR_B:
      util.irender_fill(pixels + y::ivec2{0, t[yy] / 2},
                        t / y::ivec2{1, 2}, colour);
      util.irender_fill(ur, l, r, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_DL_B:
      util.irender_fill(pixels, t / y::ivec2{1, 2}, colour);
      util.irender_fill(dl, l, r, colour);
      break;
    case Tileset::COLLIDE_SLOPEH_DR_B:
      util.irender_fill(pixels, t / y::ivec2{1, 2}, colour);
      util.irender_fill(dr, l, r, colour);
      break;
    default: {}
  }
}

// End anonymous namespace.
}

SetCollideAction::SetCollideAction(
    Tileset& tileset, const y::ivec2& coord,
    std::int32_t old_collide, std::int32_t new_collide)
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

bool SetCollideAction::is_noop() const
{
  return old_collide == new_collide;
}

TilesetPanel::TilesetPanel(Tileset& tileset, std::int32_t& collide_select,
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
      _undo_stack.new_action(std::unique_ptr<StackAction>(new SetCollideAction(
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
  set_size(_tileset.get_texture().texture.get_size());
}

void TilesetPanel::draw(RenderUtil& util) const
{
  const GlTexture2D& texture = _tileset.get_texture().texture;
  util.irender_sprite(texture, texture.get_size(), y::ivec2(), y::ivec2(),
                      0.f, colour::white);

  if (_hover >= y::ivec2()) {
    util.irender_outline(_hover * Tileset::tile_size,
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

const std::int32_t CollidePanel::entries_per_row = 8;

CollidePanel::CollidePanel(std::int32_t& collide_select)
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
    std::int32_t i = v[xx] + v[yy] * entries_per_row;
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
      std::min(entries_per_row, std::int32_t(Tileset::COLLIDE_SIZE)),
      1 + (Tileset::COLLIDE_SIZE - 1) / entries_per_row});
}

void CollidePanel::draw(RenderUtil& util) const
{
  for (std::int32_t i = 0; i < Tileset::COLLIDE_SIZE; ++i) {
     y::ivec2 v{i % entries_per_row, i / entries_per_row};
     render_collision(util, Tileset::tile_size * v, i, colour::hover);
     if (i == _collide_select) {
       util.irender_outline(Tileset::tile_size * v, Tileset::tile_size,
                            colour::select);
     }
     else if (_hover >= y::ivec2() &&
              _hover.euclidean_div(Tileset::tile_size) == v) {
       util.irender_outline(Tileset::tile_size * v, Tileset::tile_size,
                            colour::hover);
     }
  }
}

TilesetEditor::TilesetEditor(
    Filesystem& output, Databank& bank, RenderUtil& util, Tileset& tileset)
  : _output(output)
  , _bank(bank)
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
    case sf::Keyboard::S:
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
          sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
        _bank.save(_output, _tileset);
      }
      break;
    case sf::Keyboard::Escape:
      _bank.save(_output, _tileset);
      end();
      break;
    default: {}
  }
}

void TilesetEditor::update()
{
  y::roll<std::int32_t>(_collide_select, 0, Tileset::COLLIDE_SIZE);

  y::ivec2 spacer = RenderUtil::from_grid();
  const Resolution& r = _util.get_window().get_mode();

  const std::int32_t total_width =
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
