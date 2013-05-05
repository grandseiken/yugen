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
  , _map_list(y::ivec2(), y::ivec2(),
              Colour::black, Colour::item, Colour::select)
  , _cell_list(y::ivec2(), y::ivec2(),
               Colour::black, Colour::item, Colour::select)
  , _script_list(y::ivec2(), y::ivec2(),
                 Colour::black, Colour::item, Colour::select)
  , _list_select(0)
  , _tileset_select(0)
  , _map_select(0)
  , _cell_select(0)
  , _script_select(0)
{
}

void Yedit::event(const sf::Event& e)
{
  y::int32& item_select =
      _list_select == 0 ? _tileset_select :
      _list_select == 1 ? _map_select :
      _list_select == 2 ? _cell_select :
                          _script_select;

  if (e.type == sf::Event::MouseWheelMoved) {
    item_select -= e.mouseWheel.delta;
  }

  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
               sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

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
        if (shift) {
          push(y::move_unique(new TextInputModal(
              _util, "/world/new.map", _input_result, "Add map using name:")));
        }
        else {
          push(y::move_unique(new MapEditor(
              _bank, _util, _bank.maps.get(_map_select))));
        }
      }
      break;
    case sf::Keyboard::Escape:
      // TODO: work out where saving should happen.
      _bank.save_all<CellBlueprint, proto::CellBlueprint>(_output);
      _bank.save_all<CellMap, proto::CellMap>(_output);
      _bank.save_all<Tileset, proto::Tileset>(_output);
      end();
      break;
    default: {}
  }
}

void Yedit::update()
{
  y::roll<y::int32>(_list_select, 0, 4);
  y::roll<y::int32>(_tileset_select, 0, _bank.tilesets.size());
  y::roll<y::int32>(_map_select, 0, _bank.maps.size());
  y::roll<y::int32>(_cell_select, 0, _bank.cells.size());
  y::roll<y::int32>(_script_select, 0, _bank.scripts.size());

  const Resolution& screen = _util.get_window().get_mode();
  y::ivec2 size =
      (screen.size / RenderUtil::from_grid() - y::ivec2{5, 4}) / y::ivec2{4, 1};

  _tileset_list.set_origin({1, 3});
  _map_list.set_origin({2 + size[xx], 3});
  _cell_list.set_origin({3 + 2 * size[xx], 3});
  _script_list.set_origin({4 + 3 * size[xx], 3});

  _tileset_list.set_size(size);
  _map_list.set_size(size);
  _cell_list.set_size(size);
  _script_list.set_size(size);

  _tileset_list.set_panel(
      _list_select == 0 ? Colour::faint_panel : Colour::black);
  _map_list.set_panel(
      _list_select == 1 ? Colour::faint_panel : Colour::black);
  _cell_list.set_panel(
      _list_select == 2 ? Colour::faint_panel : Colour::black);
  _script_list.set_panel(
      _list_select == 3 ? Colour::faint_panel : Colour::black);

  if (_input_result.success) {
    const y::string& result = _input_result.result;
    if (!_bank.maps.is_name_used(result) &&
        result.substr(0, 7) == "/world/" &&
        result.substr(result.length() - 4) == ".map") {
      _bank.maps.insert(result, y::move_unique(new CellMap()));
    }
    _input_result.success = false;
  }
}

bool used(const Tileset& tileset, const CellBlueprint& cell)
{
  return cell.is_tileset_used(tileset);
}

bool used(const Tileset& tileset, const CellMap& map)
{
  for (auto it = map.get_cartesian(); it; ++it) {
    if (!map.is_coord_used(*it)) {
      continue;
    }
    if (used(tileset, *map.get_coord(*it))) {
      return true;
    }
  }
  return false;
}

bool used(const LuaFile& script, const CellMap& map)
{
  for (const ScriptBlueprint& s : map.get_scripts()) {
    if (script.path == s.path) {
      return true;
    }
  }
  return false;
}

bool used(const CellBlueprint& cell, const CellMap& map)
{
  for (auto it = map.get_cartesian(); it; ++it) {
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

  struct local {
    static void render_list(
        RenderUtil& util, const UiList& list, bool active,
        const y::string& title, const y::string_vector& source,
        y::vector<bool>& actives, y::size select)
    {
      util.render_text(
          title, RenderUtil::from_grid(list.get_origin() - y::ivec2{0, 2}),
          active ? Colour::select : Colour::item);
      y::vector<Colour> items;
      for (bool b : actives) {
        items.push_back(
            b ? Colour::select : active ? Colour::item : Colour::dark_outline);
      }
      list.draw(util, items, source, select);
      util.render_outline(
          RenderUtil::from_grid(list.get_origin()) - y::ivec2{1, 1},
          RenderUtil::from_grid(list.get_size()) + y::ivec2{2, 2},
          active ? Colour::outline : Colour::dark_outline);
    }
  };

  // Render tileset list.
  y::vector<bool> items;
  for (y::int32 i = 0; i < y::int32(_bank.tilesets.size()); ++i) {
    items.push_back(
        _list_select == 0 ? i == _tileset_select :
        _list_select == 1 ? used(_bank.tilesets.get(i),
                                 _bank.maps.get(_map_select)) :
        _list_select == 2 ? used(_bank.tilesets.get(i),
                                 _bank.cells.get(_cell_select)) :
                            false);
  }
  local::render_list(_util, _tileset_list, _list_select == 0, "Tilesets",
                     _bank.tilesets.get_names(), items, _tileset_select);

  // Render map list.
  items.clear();
  for (y::int32 i = 0; i < y::int32(_bank.maps.size()); ++i) {
    items.push_back(
        _list_select == 0 ? used(_bank.tilesets.get(_tileset_select),
                                 _bank.maps.get(i)) :
        _list_select == 1 ? i == _map_select :
        _list_select == 2 ? used(_bank.cells.get(_cell_select),
                                 _bank.maps.get(i)) :
                            used(_bank.scripts.get(_script_select),
                                 _bank.maps.get(i)));
  }
  local::render_list(_util, _map_list, _list_select == 1, "Maps",
                     _bank.maps.get_names(), items, _map_select);

  // Render cell list.
  items.clear();
  for (y::int32 i = 0; i < y::int32(_bank.cells.size()); ++i) {
    items.push_back(
        _list_select == 0 ? used(_bank.tilesets.get(_tileset_select),
                                 _bank.cells.get(i)) :
        _list_select == 1 ? used(_bank.cells.get(i),
                                 _bank.maps.get(_map_select)) :
        _list_select == 2 ? i == _cell_select :
                            false);
  }
  local::render_list(_util, _cell_list, _list_select == 2, "Cells",
                     _bank.cells.get_names(), items, _cell_select);

  // Render script list.
  items.clear();
  for (y::int32 i = 0; i < y::int32(_bank.scripts.size()); ++i) {
    items.push_back(
        _list_select == 0 ? false :
        _list_select == 1 ? used(_bank.scripts.get(i),
                                 _bank.maps.get(_map_select)) :
        _list_select == 2 ? false :
                            i == _script_select);
  }
  local::render_list(_util, _script_list, _list_select == 3, "Scripts",
                     _bank.scripts.get_names(), items, _script_select);
}

y::int32 main(y::int32 argc, char** argv)
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

  ModalStack stack;
  stack.push(y::move_unique(new Yedit(filesystem, databank, util)));
  stack.run(window);
  return 0;
}
