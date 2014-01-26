#ifndef YANG__INTERNAL_TYPE_H
#define YANG__INTERNAL_TYPE_H

#include "../common/math.h"
#include "../common/string.h"
#include "../common/vector.h"
#include "type.h"

namespace yang {
namespace internal {

// TYPE_ERROR is a special type assigned to expressions containing an error
// where the type cannot be determined. Further errors involving a value
// of this type are suppressed (to avoid cascading error messages).
class Type {
public:

  // When adding a new type, make sure to update:
  // - the user-facing equivalent in type.h;
  // - the type-conversions and global data-structure calculation in irgen.cpp;
  // - the trampoline function generation, also in irgen.cpp;
  // - the trampoline function call templates in type_info.h;
  // - the external to internal conversion functions in this file;
  // - all the other obvious places. There are definitely a lot.
  enum type_base {
    ERROR,
    // Temporary type to indicate a name is in scope, but inaccessible as it
    // lives in an enclosing function (and closures are not implemented).
    ENCLOSING_FUNCTION,
    VOID,
    INT,
    WORLD,
    FUNCTION,
    USER_TYPE,
  };

  // Convert from an external type.
  Type(const yang::Type& type);

  // A count greater than one constructs a vector type. This is allowed
  // only if the base is type INT or WORLD. This constructor can't create
  // FUNCTION types. Invalid construction will result in an ERROR type.
  Type(type_base base, y::size count = 1);
  // Construct FUNCTION types. Passing anything other than FUNCTION will
  // result in an error.
  Type(type_base base, const Type& return_type);
  // Construct USER_TYPE types. Passing anything other than USER_TYPE will
  // result in an error.
  Type(type_base base, const y::string& user_type_name);
  // Set const.
  void set_const(bool is_const);

  // Return components of the type.
  type_base base() const;
  y::size count() const;
  const y::string& user_type_name() const;
  bool is_const() const;
  // Return a string representation of the type.
  y::string string() const;

  // All of the following functions also return true if this is type ERROR.
  // True if this is type ERROR.
  bool is_error() const;
  // True if this is type VOID.
  bool is_void() const;
  // True if this is not type VOID.
  bool not_void() const;
  // True if this is type INT or WORLD with a count of 1.
  bool primitive() const;
  // True if this is type INT or WORLD with a count greater than 1.
  bool is_vector() const;
  // True if this is type INT (either primitive or vector).
  bool is_int() const;
  // True if this is type WORLD (either primitive or vector).
  bool is_world() const;
  // True if this is type FUNCTION (with any argument and return types).
  bool function() const;
  // True if this is any USER_TYPE.
  bool user_type() const;

  // Access the element types. For FUNCTION types, the first element is the
  // return type, and subsequent elements are argument types.
  const Type& elements(y::size index) const;
  void add_element(const Type& type);
  y::size element_size() const;
  // True if this type has the given number of elements (or is type ERROR).
  bool element_size(y::size num_elements) const;
  // True if this type has given type at the given element index, or any
  // type involved in type ERROR.
  bool element_is(y::size index, const Type& type) const;

  // True if the vector element-counts of these types allow for interaction;
  // that is, either the element-counts are the same (and they can interact
  // point-wise), or either element-count is 1 (and the value can be implicitly
  // vectorised), or either type is type ERROR.
  bool count_binary_match(const Type& t) const;
  // True if the given type is identical, or either is type ERROR.
  bool is(const Type& t) const;
  // If the given type is identical, returns it. Otherwise, returns type ERROR.
  Type unify(const Type& t) const;

  // Raw equality comparisons (ignoring ERROR). Don't use for type-checking.
  bool operator==(const Type& t) const;
  bool operator!=(const Type& t) const;

  // Convert to external type.
  yang::Type external(bool exported) const;

private:

  y::string string_internal() const;

  type_base _base;
  y::size _count;
  bool _const;
  y::vector<Type> _elements;
  y::string _user_type_name;

};

// End namespace yang::internal.
}
}

#endif
