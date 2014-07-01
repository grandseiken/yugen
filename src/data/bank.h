#ifndef DATA_BANK_H
#define DATA_BANK_H

#include "../render/gl_handle.h"
#include "../log.h"
#include "../save.h"
#include <unordered_map>
#include <vector>

class CellBlueprint;
class CellMap;
class Databank;
class Filesystem;
class GameStage;
class GlUtil;
class Tileset;

// Finds and loads resources. Template parameter T is the resource type which
// can be associated with string keys. The Dataset owns all resources and they
// will persist for its lifetime. Requesting resources by invalid names returns
// a reference to a standard missing resource.
template<typename T>
class Dataset {
public:

  Dataset(T& default_resource);
  Dataset(const Dataset&) = delete;
  Dataset& operator=(const Dataset&) = delete;

  // Save specific resource to filesystem.
  void save(Filesystem& filesystem, const Databank& bank,
            const std::string& name, bool human_readable = false) const;

  // Save all resources to filesystem.
  void save_all(Filesystem& filesystem, const Databank& bank,
                bool human_readable = false) const;

  // Add a new resource.
  void insert(const std::string& name, std::unique_ptr<T> resource);
  std::size_t size() const;
  bool empty() const;

  // Get list of loaded resource names.
  const std::vector<std::string>& get_names() const;
  // Check if a name is used.
  bool is_name_used(const std::string& name) const;

  // Get resource by name.
  const T& get(const std::string& name) const;
  /***/ T& get(const std::string& name);

  // Get resource by index.
  const T& get(std::size_t index) const;
  /***/ T& get(std::size_t index);

  // Get name resource is stored as.
  const std::string& get_name(const T& resource) const;
  const std::string& get_name(std::size_t index) const;
  // Get index resource is stored at.
  std::size_t get_index(const T& resource) const;
  std::size_t get_index(const std::string& name) const;

  // Rename resource.
  void rename(const T& resource, const std::string& new_name);

  // Clear everything.
  void clear();

private:

  typedef std::unordered_map<std::string, std::unique_ptr<T>> map_type;
  std::vector<std::string> _list;
  map_type _map;
  T& _default;

};

struct LuaFile {
  std::string path;
  std::string contents;

  // Editor display.
  y::fvec4 yedit_colour;
};

#ifndef SPRITE_DEC
#define SPRITE_DEC
struct Sprite {
  GlTexture2D texture;
  GlTexture2D normal;
};
#endif

class Databank {
public:

  ~Databank();
  Databank();
  Databank(const Filesystem& filesystem, GlUtil& gl,
           bool load_yedit_data = false);

  Databank(const Databank&) = delete;
  Databank& operator=(const Databank&) = delete;

  void reload_cells_and_maps(const Filesystem& filesystem);

  // Get the dataset for a generic type.
  template<typename T>
  const Dataset<T>& get_set() const;
  template<typename T>
  /***/ Dataset<T>& get_set();

  // Save resource of a given type.
  template<typename>
  void save_name(Filesystem& filesystem, const std::string& name) const;
  template<typename T>
  void save(Filesystem& filesystem, const T& t) const;
  // Save all the resources of a certain type.
  template<typename>
  void save_all(Filesystem& filesystem) const;

private:

  void make_default_map();
  // Load the script and call some functions to get its info.
  void make_lua_file(LuaFile& file, GameStage& gl);

  // Used to delete the textures when the Databank is destroyed.
  std::vector<GlUnique<GlTexture2D>> _textures;

  std::unique_ptr<LuaFile> _default_script;
  std::unique_ptr<Sprite> _default_sprite;
  std::unique_ptr<Tileset> _default_tileset;
  std::unique_ptr<CellBlueprint> _default_cell;
  std::unique_ptr<CellMap> _default_map;

public:

  Dataset<LuaFile> scripts;
  Dataset<Sprite> sprites;
  Dataset<Tileset> tilesets;
  Dataset<CellBlueprint> cells;
  Dataset<CellMap> maps;

};

template<typename T>
Dataset<T>::Dataset(T& default_resource)
  : _default(default_resource)
{
}

template<typename T>
void Dataset<T>::save(Filesystem& filesystem, const Databank& bank,
                      const std::string& name, bool human_readable) const
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    it->second->y::template io<typename T::io_protobuf_type>::save(
        filesystem, bank, name, human_readable);
  }
}

template<typename T>
void Dataset<T>::save_all(Filesystem& filesystem, const Databank& bank,
                          bool human_readable) const
{
  for (const std::string& name : _list) {
    save(filesystem, bank, name, human_readable);
  }
}

template<typename T>
void Dataset<T>::insert(const std::string& name, std::unique_ptr<T> resource)
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    it->second.swap(resource);
    return;
  }

  _list.emplace_back(name);
  std::sort(_list.begin(), _list.end());
  auto jt = _map.emplace(name, std::unique_ptr<T>());
  jt.first->second.swap(resource);
}

template<typename T>
std::size_t Dataset<T>::size() const
{
  return _list.size();
}

template<typename T>
bool Dataset<T>::empty() const
{
  return _list.empty();
}

template<typename T>
const std::vector<std::string>& Dataset<T>::get_names() const
{
  return _list;
}

template<typename T>
bool Dataset<T>::is_name_used(const std::string& name) const
{
  for (const std::string& s : _list) {
    if (s == name) {
      return true;
    }
  }
  return false;
}

template<typename T>
const T& Dataset<T>::get(const std::string& name) const
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    return *it->second;
  }
  log_err("Couldn't find resource ", name);
  return _default;
}

template<typename T>
T& Dataset<T>::get(const std::string& name)
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    return *it->second;
  }
  log_err("Couldn't find resource ", name);
  return _default;
}

template<typename T>
const T& Dataset<T>::get(std::size_t index) const
{
  if (index < get_names().size()) {
    return get(get_names()[index]);
  }
  log_err("Couldn't find resource ", index);
  return _default;
}

template<typename T>
T& Dataset<T>::get(std::size_t index)
{
  if (index < get_names().size()) {
    return get(get_names()[index]);
  }
  log_err("Couldn't find resource ", index);
  return _default;
}

template<typename T>
const std::string& Dataset<T>::get_name(const T& resource) const
{
  static std::string none = "<no resource>";
  for (const auto& pair : _map) {
    if (pair.second.get() == &resource) {
      return pair.first;
    }
  }
  log_err("Couldn't find name of resource");
  return none;
}

template<typename T>
const std::string& Dataset<T>::get_name(std::size_t index) const
{
  return get_name(get(index));
}

template<typename T>
std::size_t Dataset<T>::get_index(const T& resource) const
{
  for (std::size_t i = 0; i < _list.size(); ++i) {
    const std::string& s = _list[i];
    auto it = _map.find(s);
    if (it != _map.end() && it->second.get() == &resource) {
      return i;
    }
  }
  log_err("Couldn't find index of resource");
  return -1;
}

template<typename T>
std::size_t Dataset<T>::get_index(const std::string& name) const
{
  return get_index(get(name));
}

template<typename T>
void Dataset<T>::rename(const T& resource, const std::string& new_name)
{
  std::size_t i = get_index(resource);
  if (i == std::size_t(-1)) {
    return;
  }
  _list.erase(_list.begin() + i);

  const std::string& name = get_name(resource);
  auto it = _map.find(name);
  if (it == _map.end()) {
    return;
  }
  std::unique_ptr<T> temp;
  temp.swap(it->second);
  _map.erase(it);

  _list.emplace_back(new_name);
  std::sort(_list.begin(), _list.end());
  auto jt = _map.emplace(new_name, std::unique_ptr<T>());
  jt.first->second.swap(temp);
}

template<typename T>
void Dataset<T>::clear()
{
  _list.clear();
  _map.clear();
}

template<>
inline const Dataset<LuaFile>& Databank::get_set<LuaFile>() const
{
  return scripts;
}

template<>
inline Dataset<LuaFile>& Databank::get_set<LuaFile>()
{
  return scripts;
}

template<>
inline const Dataset<Sprite>& Databank::get_set<Sprite>() const
{
  return sprites;
}

template<>
inline Dataset<Sprite>& Databank::get_set<Sprite>()
{
  return sprites;
}

template<>
inline const Dataset<Tileset>& Databank::get_set<Tileset>() const
{
  return tilesets;
}

template<>
inline Dataset<Tileset>& Databank::get_set<Tileset>()
{
  return tilesets;
}

template<>
inline const Dataset<CellBlueprint>& Databank::get_set<CellBlueprint>() const
{
  return cells;
}

template<>
inline Dataset<CellBlueprint>& Databank::get_set<CellBlueprint>()
{
  return cells;
}

template<>
inline const Dataset<CellMap>& Databank::get_set<CellMap>() const
{
  return maps;
}

template<>
inline Dataset<CellMap>& Databank::get_set<CellMap>()
{
  return maps;
}

template<typename T>
void Databank::save_name(Filesystem& filesystem, const std::string& name) const
{
  get_set<T>().save(filesystem, *this, name);
}

template<typename T>
void Databank::save(Filesystem& filesystem, const T& t) const
{
  save_name<T>(filesystem, get_set<T>().get_name(t));
}

template<typename T>
void Databank::save_all(Filesystem& filesystem) const
{
  get_set<T>().save_all(filesystem, *this);
}

#endif
