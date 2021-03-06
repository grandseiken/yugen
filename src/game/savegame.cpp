#include "savegame.h"

#include "../data/bank.h"
#include "../../gen/proto/savegame.pb.h"

// Possibly these two functions should be in lua_types.h, but they're unlikely
// to be useful in any other context.
namespace {

void save_to_proto(const LuaValue& value, proto::Value& proto)
{
  if (value.type == LuaValue::WORLD) {
    proto.set_type(proto::WORLD);
    proto.set_world_value(value.world);
  }
  else if (value.type == LuaValue::BOOLEAN) {
    proto.set_type(proto::BOOLEAN);
    proto.set_boolean_value(value.boolean);
  }
  else if (value.type == LuaValue::STRING) {
    proto.set_type(proto::STRING);
    proto.set_string_value(value.string);
  }
  else if (value.type == LuaValue::ARRAY) {
    proto.set_type(proto::ARRAY);
    for (const auto& v : value.array) {
      auto proto_v = proto.add_array_value();
      save_to_proto(v, *proto_v);
    }
  }
  else {
    log_err("Saving savegame value with invalid type");
  }
}

void load_from_proto(LuaValue& value, const proto::Value& proto)
{
  if (proto.type() == proto::WORLD) {
    value.type = LuaValue::WORLD;
    value.world = proto.world_value();
  }
  else if (proto.type() == proto::BOOLEAN) {
    value.type = LuaValue::BOOLEAN;
    value.boolean = proto.boolean_value();
  }
  else if (proto.type() == proto::STRING) {
    value.type = LuaValue::STRING;
    value.string = proto.string_value();
  }
  else if (proto.type() == proto::ARRAY) {
    value.type = LuaValue::ARRAY;
    while (std::int32_t(value.array.size()) < proto.array_value_size()) {
      value.array.emplace_back(0.0);
    }
    for (std::int32_t i = 0; i < proto.array_value_size(); ++i) {
      load_from_proto(value.array[i], proto.array_value(i));
    }
  }
  else {
    log_err("Loading savegame value with invalid type");
  }
}

// End anonymous namespace.
}

Savegame::Savegame()
  : _default_value(false)
{
}

void Savegame::save(Filesystem& filesystem,
                    const std::string& path, bool human_readable) const
{
  Databank temp;
  y::io<proto::Savegame>::save(filesystem, temp, path, human_readable);
}

void Savegame::load(const Filesystem& filesystem,
                    const std::string& path, bool human_readable)
{
  Databank temp;
  y::io<proto::Savegame>::load(filesystem, temp, path, human_readable);
}

void Savegame::clear()
{
  _map.clear();
}

const LuaValue& Savegame::get(const std::string& key) const
{
  auto it = _map.find(key);
  if (it == _map.end()) {
    return _default_value;
  }
  return it->second;
}

void Savegame::put(const std::string& key, const LuaValue& value)
{
  // We can't handle arbitrary userdata types. It might be possible to provide
  // an exception for two-dimensional vectors for convenience.
  if (value.type == LuaValue::USERDATA) {
    log_err("Tried to write userdata into savegame");
    return;
  }
  _map.erase(key);
  _map.emplace(key, value);
}

void Savegame::save_to_proto(const Databank&, proto::Savegame& proto) const
{
  for (const auto& pair : _map) {
    auto entry = proto.add_entries();
    entry->set_key(pair.first);
    ::save_to_proto(pair.second, *entry->mutable_value());
  }
}

void Savegame::load_from_proto(Databank&, const proto::Savegame& proto)
{
  clear();

  for (std::int32_t i = 0; i < proto.entries_size(); ++i) {
    LuaValue value(0.0);
    ::load_from_proto(value, proto.entries(i).value());
    put(proto.entries(i).key(), value);
  }
}
