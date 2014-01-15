#ifndef YANG__TYPE_H
#define YANG__TYPE_H

#include "../common/math.h"
#include "../common/string.h"
#include "../common/vector.h"

namespace yang {

namespace internal {
  class Type;
  template<typename T>
  struct TypeInfo;
}

class Type {
public:

  // Return a string representation of the type.
  y::string string() const;

  // Type qualifiers.
  bool is_exported() const;
  bool is_const() const;

  // Singular types.
  bool is_void() const;
  bool is_int() const;
  bool is_world() const;

  // Int and world vector types.
  bool is_vector() const;
  bool is_int_vector() const;
  bool is_world_vector() const;
  y::size get_vector_size() const;

  // Function types.
  bool is_function() const;
  y::size get_function_num_args() const;
  const Type& get_function_return_type() const;
  const Type& get_function_arg_type(y::size index) const;

  bool operator==(const Type& t) const;
  bool operator!=(const Type& t) const;

private:

  friend class internal::Type;
  template<typename T>
  friend struct internal::TypeInfo;
  Type();

  enum type_base {
    VOID,
    INT,
    WORLD,
    FUNCTION,
  };

  bool _exported;
  bool _const;
  type_base _base;
  y::size _count;
  y::vector<Type> _elements;
  static Type void_type;

};

// End namespace yang.
}

#endif
