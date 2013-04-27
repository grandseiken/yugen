#include "ui_util.h"

UiList::UiList(const y::ivec2& origin, const y::ivec2& size,
               const Colour& panel, const Colour& item, const Colour& s_item)
  : _origin(origin)
  , _size(size)
  , _panel(panel)
  , _item(item)
  , _s_item(s_item)
{
}

void UiList::set_origin(const y::ivec2& origin)
{
  _origin = origin;
}

void UiList::set_size(const y::ivec2& size)
{
  _size = size;
}

const y::ivec2& UiList::get_origin() const
{
  return _origin;
}

const y::ivec2& UiList::get_size() const
{
  return _size;
}

void UiList::draw(RenderUtil& util, const y::vector<Colour>& items,
                  const y::string_vector& source, y::size select) const
{
  util.render_fill_grid(_origin, _size, _panel);

  y::int32 w = _size[xx];
  y::int32 h = _size[yy];
  y::int32 offset = y::max<y::int32>(0,
    select >= source.size() - h / 2 - 1 ? source.size() - h : select - h / 2);

  for (y::int32 i = 0; i < h; ++i) {
    if (i + offset >= y::int32(items.size()) ||
        i + offset >= y::int32(source.size())) {
      break;
    }

    y::string s = source[i + offset];
    s = y::int32(s.length()) > w ? s.substr(0, w - 3) + "..." : s;
    util.render_text_grid(s, _origin + y::ivec2{0, i}, items[i + offset]);
  }
}

void UiList::draw(RenderUtil& util,
                  const y::string_vector& source, y::size select) const
{
  y::vector<Colour> items;
  for (y::size n = 0; n < source.size(); ++n) {
    items.push_back(n == select ? _s_item : _item);
  }

  draw(util, items, source, select);
}
