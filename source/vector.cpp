#include "vector.h"
#include "proto/common.pb.h"

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

template<bool B>
const y::element_accessor<0> element_accessor_instance<B>::x;
template<bool B>
const y::element_accessor<1> element_accessor_instance<B>::y;
template<bool B>
const y::element_accessor<2> element_accessor_instance<B>::z;
template<bool B>
const y::element_accessor<3> element_accessor_instance<B>::w;
