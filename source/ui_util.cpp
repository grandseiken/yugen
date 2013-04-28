#include "ui_util.h"
#include "window.h"

#include <SFML/Window.hpp>

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

TextInputResult::TextInputResult()
  : success(false)
  , result("")
{
}

TextInputModal::TextInputModal(RenderUtil& util, const y::string& default_text,
                               TextInputResult& output, const y::string& label)
  : _util(util)
  , _output(output)
  , _label(label)
  , _cursor(default_text.length())
{
  output.success = false;
  output.result = default_text;
}

void TextInputModal::event(const sf::Event& e)
{
  static const y::string allowed =
      y::string("abcdefghijklmnopqrstuvwxyz") +
      y::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ") +
      y::string("0123456789[](){}_-+=@#&./ ");
  if (e.type == sf::Event::TextEntered && e.text.unicode < 128 &&
      allowed.find_first_of(char(e.text.unicode)) != y::string::npos) {
    _output.result = _output.result.substr(0, _cursor) +
        char(e.text.unicode) + _output.result.substr(_cursor);
    ++_cursor;
  }
  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  switch (e.key.code) {
    case sf::Keyboard::Left:
      if (_cursor) {
        --_cursor;
      }
      break;
    case sf::Keyboard::Right:
      if (_cursor < _output.result.size()) {
        ++_cursor;
      }
      break;
    case sf::Keyboard::BackSpace:
      if (_cursor) {
        _output.result = _output.result.substr(0, _cursor - 1) +
            _output.result.substr(_cursor);
        --_cursor;
      }
      break;
    case sf::Keyboard::Delete:
      if (_cursor < _output.result.size()) {
        _output.result = _output.result.substr(0, _cursor) +
            _output.result.substr(_cursor + 1);
      }
      break;
    case sf::Keyboard::Return:
      _output.success = true;
      end();
      break;
    case sf::Keyboard::Escape:
      _output.success = false;
      end();
      break;
    default: {}
  }
}

void TextInputModal::update()
{
}

void TextInputModal::draw() const
{
  const Resolution& r = _util.get_window().get_mode();
  y::ivec2 size = RenderUtil::from_grid({
      y::int32(y::max(_label.length(), 1 + _output.result.length())), 2});
  y::ivec2 origin = r.size / 2 - size / 2;
  _util.render_fill(origin, {size[xx], RenderUtil::from_grid()[yy]},
                    Colour::panel);
  _util.render_outline(origin - y::ivec2{1, 1},
                       y::ivec2{2, 2} + size, Colour::outline);
  _util.render_fill(origin + RenderUtil::from_grid({y::int32(_cursor), 1}),
                    RenderUtil::from_grid(), Colour::item);
  _util.render_text(_label, origin, Colour::item);
  _util.render_text(_output.result, origin + RenderUtil::from_grid({0, 1}),
                    Colour::select);
  if (_cursor < _output.result.length()) {
    _util.render_text(_output.result.substr(_cursor, 1),
                      origin + RenderUtil::from_grid({y::int32(_cursor), 1}),
                      Colour::black);
  }
}
