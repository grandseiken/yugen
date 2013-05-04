#include "yedit.h"

#include "databank.h"
#include "gl_util.h"
#include "map_editor.h"
#include "physical_filesystem.h"
#include "proto/cell.pb.h"
#include "render_util.h"
#include "tileset_editor.h"
#include "window.h"

#include <SFML/Window.hpp>

Yedit::Yedit(Filesystem& output, Databank& bank, RenderUtil& util)
  : _output(output)
  , _bank(bank)
  , _util(util)
  , _tileset_list(y::ivec2(), y::ivec2(),
                  Colour::black, Colour::item, Colour::select)
  , _cell_list(y::ivec2(), y::ivec2(),
               Colour::black, Colour::item, Colour::select)
  , _map_list(y::ivec2(), y::ivec2(),
              Colour::black, Colour::item, Colour::select)
  , _list_select(0)
  , _tileset_select(0)
  , _map_select(0)
  , _cell_select(0)
{
}

void Yedit::event(const sf::Event& e)
{
  y::int32& item_select =
      _list_select == 0 ? _tileset_select :
      _list_select == 1 ? _map_select :
                          _cell_select;

  if (e.type == sf::Event::MouseWheelMoved) {
    item_select -= e.mouseWheel.delta;
  }

  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  switch (e.key.code) {
    case sf::Keyboard::S:
    case sf::Keyboard::Down:
      ++item_select;
      break;
    case sf::Keyboard::W:
    case sf::Keyboard::Up:
      --item_select;
      break;  
    case sf::Keyboard::Tab:
    case sf::Keyboard::D:
    case sf::Keyboard::Right:
      ++_list_select;
      break;
    case sf::Keyboard::A:
    case sf::Keyboard::Left:
      --_list_select;
      break;
    case sf::Keyboard::Return:
      if (_list_select == 0) {
        push(y::move_unique(new TilesetEditor(
            _bank, _util, _bank.tilesets.get(_tileset_select))));
      }
      else if (_list_select == 1) {
        push(y::move_unique(new MapEditor(
            _bank, _util, _bank.maps.get(_map_select))));
      }
      break;
    case sf::Keyboard::Escape:
      // TODO: work out where saving should happen.
      _bank.save_all<CellBlueprint, proto::CellBlueprint>(_output);
      _bank.save_all<CellMap, proto::CellMap>(_output);
      end();
      break;
    default: {}
  }
}

void Yedit::update()
{
  y::roll<y::int32>(_list_select, 0, 3);
  y::roll<y::int32>(_tileset_select, 0, _bank.tilesets.size());
  y::roll<y::int32>(_map_select, 0, _bank.maps.size());
  y::roll<y::int32>(_cell_select, 0, _bank.cells.size());

  const Resolution& screen = _util.get_window().get_mode();
  y::ivec2 size = (screen.size / RenderUtil::from_grid() - y::ivec2{4, 4}) / 3;

  _tileset_list.set_origin({1, 3});
  _map_list.set_origin({2 + size[xx], 3});
  _cell_list.set_origin({3 + 2 * size[xx], 3});

  _tileset_list.set_size(size);
  _map_list.set_size(size);
  _cell_list.set_size(size);

  _tileset_list.set_panel(
      _list_select == 0 ? Colour::faint_panel : Colour::black);
  _map_list.set_panel(_list_select == 1 ? Colour::faint_panel : Colour::black);
  _cell_list.set_panel(_list_select == 2 ? Colour::faint_panel : Colour::black);
}

bool used(const Tileset& tileset, const CellBlueprint& cell)
{
  return cell.is_tileset_used(tileset);
}

bool used(const Tileset& tileset, const CellMap& map)
{
  for (auto it = y::cartesian(map.get_boundary_min(),
                              map.get_boundary_max()); it; ++it) {
    if (!map.is_coord_used(*it)) {
      continue;
    }
    if (used(tileset, *map.get_coord(*it))) {
      return true;
    }
  }
  return false;
}

bool used(const CellBlueprint& cell, const CellMap& map)
{
  for (auto it = y::cartesian(map.get_boundary_min(),
                              map.get_boundary_max()); it; ++it) {
    if (map.get_coord(*it) == &cell) {
      return true;
    }
  }
  return false;
}

void Yedit::draw() const
{
  _util.get_gl().bind_window(true, true);
  const Resolution& screen = _util.get_window().get_mode();
  _util.set_resolution(screen.size);

  _util.render_text_grid(
      "Tilesets", {1, 1},
      _list_select == 0 ? Colour::select : Colour::item);
  _util.render_text_grid(
      "Maps", {2 + _tileset_list.get_size()[xx], 1},
      _list_select == 1 ? Colour::select : Colour::item);
  _util.render_text_grid(
      "Cells", {3 + (_tileset_list.get_size() + _cell_list.get_size())[xx], 1},
      _list_select == 2 ? Colour::select : Colour::item);

  // Render tileset list.
  y::vector<Colour> items;
  for (y::int32 i = 0; i < y::int32(_bank.tilesets.size()); ++i) {
    bool b =
        _list_select == 0 ? i == _tileset_select :
        _list_select == 1 ? used(_bank.tilesets.get(i),
                                 _bank.maps.get(_map_select)) :
                            used(_bank.tilesets.get(i),
                                 _bank.cells.get(_cell_select));
    items.push_back(b ? Colour::select : Colour::item);
  }
  _tileset_list.draw(_util, items,
                     _bank.tilesets.get_names(), _tileset_select);

  // Render map list.
  items.clear();
  for (y::int32 i = 0; i < y::int32(_bank.maps.size()); ++i) {
    bool b =
        _list_select == 0 ? used(_bank.tilesets.get(_tileset_select),
                                 _bank.maps.get(i)) :
        _list_select == 1 ? i == _map_select :
                            used(_bank.cells.get(_cell_select),
                                 _bank.maps.get(i));
    items.push_back(b ? Colour::select : Colour::item);
  }
  _map_list.draw(_util, items,
                 _bank.maps.get_names(), _map_select);

  // Render cell list.
  items.clear();
  for (y::int32 i = 0; i < y::int32(_bank.cells.size()); ++i) {
    bool b =
        _list_select == 0 ? used(_bank.tilesets.get(_tileset_select),
                                 _bank.cells.get(i)) :
        _list_select == 1 ? used(_bank.cells.get(i),
                                 _bank.maps.get(_map_select)) :
                            i == _cell_select;
    items.push_back(b ? Colour::select : Colour::item);
  }
  _cell_list.draw(_util, items,
                  _bank.cells.get_names(), _cell_select);
}

// TODO: testing.
#include "lua.h"
int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  Window window("Crunk Yedit", 24, {0, 0}, false, true);
  PhysicalFilesystem filesystem("data");
  GlUtil gl(filesystem, window);
  if (!gl) {
    return 1;
  }
  Databank databank(filesystem, gl);
  RenderUtil util(gl);

  // TODO: testing.
  LuaFile& lua_file = databank.scripts.get("/scripts/hello.lua");
  Script s(lua_file.path, lua_file.contents);
  s.call("test");
  s.call("test");

  ModalStack stack;
  stack.push(y::move_unique(new Yedit(filesystem, databank, util)));
  stack.run(window);
  return 0;
}
