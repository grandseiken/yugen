#include "vec.h"
#include "../gen/proto/common.pb.h"

namespace y {

  void save_to_proto(const y::ivec2& v, proto::ivec2& proto)
  {
    proto.set_x(v[xx]);
    proto.set_y(v[yy]);
  }

  void load_from_proto(y::ivec2& v, const proto::ivec2& proto)
  {
    v[xx] = proto.x();
    v[yy] = proto.y();
  }

}
