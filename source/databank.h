#ifndef DATABANK_H
#define DATABANK_H

#include "common.h"
#include "cell.h"
#include "tileset.h"

#include <algorithm>
#include <iostream>

class Databank;
class Filesystem;
class GlUtil;

// Globs and loads resources. Requesting resources by invalid names returns
// a reference to a standard missing resource.
// TODO: exceptions?
template<typename T>
class Dataset : public y::no_copy {
public:

  Dataset(T& default_resource);

  // Save all resources to filesystem.
  template<typename P,
           typename std::enable_if<
               std::is_base_of<y::io<P>, T>::value, int>::type = 0>
  void save_all(Filesystem& filesystem, const Databank& bank,
                bool human_readable = false) const;

  // Add a new resource.
  void insert(const y::string& name, y::unique<T> resource);
  y::size size() const;
  bool empty() const;

  // Get list of loaded resource names.
  const y::string_vector& get_names() const;

  // Get resource by name.
  const T& get(const y::string& name) const;
  /***/ T& get(const y::string& name);

  // Get resource by index.
  const T& get(y::size index) const;
  /***/ T& get(y::size index);

  // Get name resource is stored as.
  const y::string& get_name(const T& resource) const;
  // Get index resource is stored at.
  y::size get_index(const T& resource) const;

private:

  typedef y::map<y::string, y::unique<T>> map_type;
  y::string_vector _list;
  map_type _map;
  T& _default;

};

class Databank : public y::no_copy {

  Tileset _default_tileset;
  CellBlueprint _default_cell;
  CellMap _default_map;

public:

  Databank(const Filesystem& filesystem, GlUtil& gl);

  // Get the dataset for a generic type.
  template<typename T>
  const Dataset<T>& get_set() const;
  template<typename T>
  /***/ Dataset<T>& get_set();

  // Save all the resources of a certain type. Pass the protobuf type as the
  // second parameter.
  template<typename T, typename P>
  void save_all(Filesystem& filesystem) const;

  Dataset<Tileset> tilesets;
  Dataset<CellBlueprint> cells;
  Dataset<CellMap> maps;

private:

  void make_default_map();

};

template<typename T>
Dataset<T>::Dataset(T& default_resource)
  : _default(default_resource)
{
}

template<typename T>
template<typename P,
         typename std::enable_if<
             std::is_base_of<y::io<P>, T>::value, int>::type>
void Dataset<T>::save_all(Filesystem& filesystem, const Databank& bank,
                          bool human_readable) const
{
  for (const y::string& s : _list) {
    auto it = _map.find(s);
    if (it == _map.end()) {
      continue;
    }
    it->second->y::io<P>::save(filesystem, bank, s, human_readable);
  }
}

template<typename T>
void Dataset<T>::insert(const y::string& name, y::unique<T> resource)
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    it->second.swap(resource);
    return;
  }

  _list.push_back(name);
  std::sort(_list.begin(), _list.end());
  auto jt = _map.insert(y::make_pair(name, y::unique<T>()));
  jt.first->second.swap(resource);
}

template<typename T>
y::size Dataset<T>::size() const
{
  return _list.size();
}

template<typename T>
bool Dataset<T>::empty() const
{
  return _list.empty();
}

template<typename T>
const y::string_vector& Dataset<T>::get_names() const
{
  return _list;
}

template<typename T>
const T& Dataset<T>::get(const y::string& name) const
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    return *it->second;
  }
  std::cerr << "Couldn't find resource " << name << std::endl;
  return _default;
}

template<typename T>
T& Dataset<T>::get(const y::string& name)
{
  auto it = _map.find(name);
  if (it != _map.end()) {
    return *it->second;
  }
  std::cerr << "Couldn't find resource " << name << std::endl;
  return _default;
}

template<typename T>
const T& Dataset<T>::get(y::size index) const
{
  if (index < get_names().size()) {
    return get(get_names()[index]);
  }
  return _default;
}

template<typename T>
T& Dataset<T>::get(y::size index)
{
  if (index < get_names().size()) {
    return get(get_names()[index]);
  }
  return _default;
}

template<typename T>
const y::string& Dataset<T>::get_name(const T& resource) const
{
  static y::string none = "<no resource>";
  for (const auto& pair : _map) {
    if (pair.second.get() == &resource) {
      return pair.first;
    }
  }
  return none;
}

template<typename T>
y::size Dataset<T>::get_index(const T& resource) const
{
  for (y::size i = 0; i < _list.size(); ++i) {
    const y::string& s = _list[i];
    auto it = _map.find(s);
    if (it != _map.end() && it->second.get() == &resource) {
      return i;
    }
  }
  return -1;
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

template<typename T, typename P>
void Databank::save_all(Filesystem& filesystem) const
{
  get_set<T>().save_all<P>(filesystem, *this);
}
#endif
