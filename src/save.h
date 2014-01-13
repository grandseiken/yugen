#ifndef SAVE_H
#define SAVE_H

#include "filesystem/filesystem.h"
#include <google/protobuf/text_format.h>

class Databank;

namespace y {

// Template for writing / reading protobufs. Classes should derive from io<P>,
// where P is the corresponding protobuf type, and implement save_to_proto and
// load_from_proto.
template<typename P>
class io : public y::no_copy {
public:

  // Allows derived classes to determine the protobuf type.
  typedef P io_protobuf_type;

  // Save this object to the filesystem, under the given path.
  // If human_readable is true, use the ASCII protobuf format.
  void save(Filesystem& filesystem, const Databank& bank,
            const y::string& path, bool human_readable = false) const;
  void load(const Filesystem& filesystem, Databank& bank,
            const y::string& path, bool human_readable = false);

protected:

  // Override to convert to and from protobuffers. These functions can use
  // the Databank to convert referenced objects to and from reference
  // filenames.
  // However, the bank is assumed to already have loaded any resources
  // we need to reference, so this doesn't support cyclic dependencies.
  virtual void save_to_proto(const Databank& bank, P& proto) const = 0;
  virtual void load_from_proto(Databank& bank, const P& proto) = 0;

};

template<typename P>
void io<P>::save(Filesystem& filesystem, const Databank& bank,
                 const y::string& path, bool human_readable) const
{
  P proto;
  save_to_proto(bank, proto);
  y::string s;
  if (human_readable) {
    google::protobuf::TextFormat::PrintToString(proto, &s);
  }
  else {
    proto.SerializeToString(&s);
  }
  filesystem.write_file(s, path);
}

template<typename P>
void io<P>::load(const Filesystem& filesystem, Databank& bank,
                 const y::string& path, bool human_readable)
{
  y::string s;
  filesystem.read_file(s, path);

  P proto;
  if (human_readable) {
    google::protobuf::TextFormat::ParseFromString(s, &proto);
  }
  else {
    proto.ParseFromString(s);
  }
  load_from_proto(bank, proto);
}

// End namespace y.
}

#endif
