#ifndef UI_UTIL_H
#define UI_UTIL_H

#include "common.h"
#include "render_util.h"
#include "vector.h"

class UiList {
public:

  UiList(const y::ivec2& origin, const y::ivec2& size,
         const Colour& panel, const Colour& item, const Colour& s_item);

  void set_origin(const y::ivec2& origin);
  void set_size(const y::ivec2& size);

  const y::ivec2& get_origin() const;
  const y::ivec2& get_size() const;

  // Draw with colour lookup from list.
  void draw(RenderUtil& util, const y::vector<Colour>& items,
            const y::string_vector& source, y::size select) const;

  // Draw with default colour values.
  void draw(RenderUtil& util,
            const y::string_vector& source, y::size select) const;


private:

  y::ivec2 _origin;
  y::ivec2 _size;
  Colour _panel;
  Colour _item;
  Colour _s_item;

};

#endif
