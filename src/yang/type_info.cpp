#include "type_info.h"
#include "type.h"

namespace yang {
namespace internal {

yang::Type TypeInfo<void>::representation() const
{
  yang::Type t;
  return t;
}

yang::Type TypeInfo<yang::int32>::representation() const
{
  yang::Type t;
  t._base = yang::Type::INT;
  return t;
}

yang::Type TypeInfo<yang::world>::representation() const
{
  yang::Type t;
  t._base = yang::Type::WORLD;
  return t;
}

// End namespace yang::internal.
}
}
