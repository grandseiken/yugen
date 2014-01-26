#include "bank.h"
#include "cell.h"
#include "tileset.h"

#include "../../gen/proto/cell.pb.h"

namespace {

y::size to_internal_index(y::int32 layer, const y::ivec2& v)
{
  return v[yy] * Cell::cell_width + v[xx] +
      (layer + Cell::background_layers) *
          Cell::cell_width * Cell::cell_height;
}

// Increment or decrement a value in a map, such that an entry exists iff its
// value is nonzero.
template<typename T, typename U>
void change_by_one(y::map<T, U>& map, const T& t, bool increment)
{
  if (!t) {
    return;
  }

  auto it = map.find(t);
  if (it != map.end()) {
    if (increment) {
      ++it->second;
    }
    else {
      --it->second;
    }
    if (!it->second) {
      map.erase(it);
    }
  }
  else if (increment || T(-1) < T(0)) {
    map[t] = increment ? 1 : -1;
  }
}

// End anonymous namespace.
}

CellMap::CellMap()
  : _min(0, 0)
  , _max(0, 0)
{
}

bool CellMap::is_coord_used(const y::ivec2& cell_coord) const
{
  return _map.find(cell_coord) != _map.end();
}

void CellMap::clear_coord(const y::ivec2& cell_coord)
{
  auto it = _map.find(cell_coord);
  if (it != _map.end()) {
    _map.erase(it);
  }
  _boundary_dirty = true;
}

void CellMap::set_coord(const y::ivec2& cell_coord, CellBlueprint& blueprint)
{
  _map[cell_coord] = &blueprint;
  _boundary_dirty = true;
}

const y::ivec2& CellMap::get_boundary_min() const
{
  recalculate_boundary();
  return _min;
}

const y::ivec2& CellMap::get_boundary_max() const
{
  recalculate_boundary();
  return _max;
}

y::ivec2_iterator CellMap::get_cartesian() const
{
  recalculate_boundary();
  return y::cartesian(_min, _max);
}

void CellMap::add_script(const ScriptBlueprint& blueprint)
{
  if (!has_script(blueprint)) {
    _scripts.emplace_back(blueprint);
  }
}

bool CellMap::has_script(const ScriptBlueprint& blueprint) const
{
  for (const ScriptBlueprint& b : _scripts) {
    if (b.min == blueprint.min && b.max == blueprint.max &&
        b.path == blueprint.path) {
      return true;
    }
  }
  return false;
}

void CellMap::remove_script(const ScriptBlueprint& blueprint)
{
  for (auto it = _scripts.begin(); it != _scripts.end();) {
    if (it->min == blueprint.min && it->max == blueprint.max &&
        it->path == blueprint.path) {
      it = _scripts.erase(it);
    }
    else {
      ++it;
    }
  }
}

const CellMap::script_list& CellMap::get_scripts() const
{
  return _scripts;
}

bool CellMap::has_script_at(const y::ivec2& v) const
{
  for (const ScriptBlueprint& b : _scripts) {
    if (y::in_region(v, b.min, y::ivec2{1, 1} + b.max - b.min)) {
      return true;
    }
  }
  return false;
}

const ScriptBlueprint& CellMap::get_script_at(const y::ivec2& v) const
{
  static ScriptBlueprint missing{y::ivec2(), y::ivec2(),
                                 "/yedit/missing.lua"};
  // We prefer the smaller Scripts as this is much nicer in the editor.
  y::int32 min;
  bool first = true;
  const ScriptBlueprint* result = y::null;
  for (const ScriptBlueprint& b : _scripts) {
    y::ivec2 len = b.max - b.min;
    if (y::in_region(v, b.min, y::ivec2{1, 1} + len)) {
      if (first || len[xx] * len[yy] <= min) {
        min = len[xx] * len[yy];
        result = &b;
        first = false;
      }
    }
  }
  if (result) {
    return *result;
  }
  return missing;
}

void CellMap::save_to_proto(const Databank& bank, proto::CellMap& proto) const
{
  for (const auto& pair : _map) {
    auto coord = proto.add_coords();
    y::save_to_proto(pair.first, *coord->mutable_coord());
    coord->set_cell(bank.cells.get_name(*pair.second));
  }

  for (const ScriptBlueprint& s : _scripts) {
    auto script = proto.add_scripts();
    y::save_to_proto(s.min, *script->mutable_min());
    y::save_to_proto(s.max, *script->mutable_max());
    script->set_path(s.path);
  }
}

void CellMap::load_from_proto(Databank& bank, const proto::CellMap& proto)
{
  for (y::int32 i = 0; i < proto.coords_size(); ++i) {
    y::ivec2 v;
    y::load_from_proto(v, proto.coords(i).coord());
    const y::string& path = proto.coords(i).cell();
    if (bank.cells.is_name_used(path)) {
      set_coord(v, bank.cells.get(path));
    }
    else {
      log_err("Map uses missing cell ", path, ", skipping");
    }
  }

  for (y::int32 i = 0; i < proto.scripts_size(); ++i) {
    ScriptBlueprint s;
    y::load_from_proto(s.min, proto.scripts(i).min());
    y::load_from_proto(s.max, proto.scripts(i).max());
    s.path = proto.scripts(i).path();
    if (bank.scripts.is_name_used(s.path)) {
      _scripts.emplace_back(s);
    }
    else {
      log_err("Map uses missing script ", s.path, ", skipping");
    }
  }
}

void CellMap::recalculate_boundary() const
{
  if (!_boundary_dirty) {
    return;
  }

  if (_map.empty()) {
    _min = {0, 0};
    _max = {0, 0};
    return;
  }

  bool first = true;
  for (const auto& pair : _map) {
    if (first) {
      _max = y::ivec2{1, 1} + pair.first;
      _min = pair.first;
    }
    else {
      _max = y::max(_max, y::ivec2{1, 1} + pair.first);
      _min = y::min(_min, pair.first);
    }
    first = false;
  }
  _boundary_dirty = false;
}

const CellBlueprint* CellMap::get_coord(const y::ivec2& cell_coord) const
{
  auto it = _map.find(cell_coord);
  return it != _map.end() ? it->second : y::null;
}

CellBlueprint* CellMap::get_coord(const y::ivec2& cell_coord)
{
  auto it = _map.find(cell_coord);
  return it != _map.end() ? it->second : y::null;
}

Tile::Tile(const Tileset* tileset, y::size index)
  : tileset(tileset)
  , index(index)
{
}

bool Tile::operator==(const Tile& tile) const
{
  return (tileset == 0 && tile.tileset == 0) ||
      (tileset == tile.tileset && index == tile.index);
}

bool Tile::operator!=(const Tile& tile) const
{
  return !operator==(tile);
}

y::int32 Tile::get_collision() const
{
  if (!tileset) {
    return Tileset::COLLIDE_NONE;
  }
  return tileset->get_collision(index);
}

const y::ivec2 Cell::cell_size{Cell::cell_width, Cell::cell_height};

Cell::Cell(const CellBlueprint& blueprint)
  : _blueprint(blueprint)
{
}

const Tile& Cell::get_tile(y::int32 layer, const y::ivec2& v) const
{
  auto it = _changed_tiles.find(to_internal_index(layer, v));
  if (it != _changed_tiles.end()) {
    return it->second;
  }
  return _blueprint.get_tile(layer, v);
}

void Cell::set_tile(y::int32 layer, const y::ivec2& v, const Tile& tile)
{
  change_by_one(_changed_tilesets, tile.tileset, true);

  y::size internal_index = to_internal_index(layer, v);
  auto it = _changed_tiles.find(internal_index);

  // Keep track of which tilesets are used in the diff.
  if (it != _changed_tiles.end()) {
    change_by_one(_changed_tilesets, it->second.tileset, false);

    if (tile == _blueprint.get_tile(layer, v)) {
      _changed_tiles.erase(it);
      return;
    }
    it->second = tile;
  }
  else {
    change_by_one(_changed_tilesets,
                  _blueprint.get_tile(layer, v).tileset, false);

    _changed_tiles[internal_index] = tile;
  }
}

bool Cell::is_tileset_used(const Tileset& tileset) const
{
  auto it = _changed_tilesets.find(&tileset);
  y::int32 change_diff = it != _changed_tilesets.end() ? it->second : 0;
  return change_diff + _blueprint.get_tileset_use_count(tileset) > 0;
}

CellBlueprint::CellBlueprint()
  : _tiles(new Tile[raw_size])
{
}

const Tile& CellBlueprint::get_tile(y::int32 layer, const y::ivec2& v) const
{
  return _tiles[to_internal_index(layer, v)];
}

void CellBlueprint::set_tile(y::int32 layer, const y::ivec2& v,
                             const Tile& tile)
{
  set_tile_internal(to_internal_index(layer, v), tile);
}

bool CellBlueprint::is_tileset_used(const Tileset& tileset) const
{
  return get_tileset_use_count(tileset);
}

y::size CellBlueprint::get_tileset_use_count(const Tileset& tileset) const
{
  auto it = _tilesets.find(&tileset);
  return it != _tilesets.end() ? it->second : 0;
}

void CellBlueprint::save_to_proto(const Databank& bank,
                                  proto::CellBlueprint& proto) const
{
  y::map<const Tileset*, y::size> map;
  y::size n = 0;
  for (const auto& pair : _tilesets) {
    if (!pair.second || !pair.first) {
      continue;
    }
    map.insert(y::make_pair(pair.first, n++));
    proto.add_tilesets(bank.tilesets.get_name(*pair.first));
  }

  for (y::int32 i = 0; i < raw_size; ++i) {
    auto tile = proto.add_tiles();
    tile->set_tileset(_tiles[i].tileset ? map[_tiles[i].tileset] : -1);
    tile->set_index(_tiles[i].index);
  }
}

void CellBlueprint::load_from_proto(Databank& bank,
                                    const proto::CellBlueprint& proto)
{
  y::vector<const Tileset*> vector;
  for (y::int32 i = 0; i < proto.tilesets_size(); ++i) {
    vector.emplace_back(&bank.tilesets.get(proto.tilesets(i)));
  }
  for (y::int32 i = 0; i < proto.tiles_size(); ++i) {
    // Handle empty cells.
    if (vector.empty()) {
      set_tile_internal(y::size(i), Tile(y::null, 0));
      continue;
    }
    set_tile_internal(y::size(i), Tile(
        proto.tiles(i).tileset() >= 0 ?
            vector[proto.tiles(i).tileset()] : y::null,
        proto.tiles(i).index()));
  }
}

void CellBlueprint::set_tile_internal(y::size internal_index, const Tile& tile)
{
  change_by_one(_tilesets, _tiles[internal_index].tileset, false);
  _tiles[internal_index] = tile;
  change_by_one(_tilesets, tile.tileset, true);
}
