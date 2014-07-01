#include "ui_util.h"
#include "render/window.h"

#include <SFML/Window.hpp>

UiList::UiList(const y::ivec2& origin, const y::ivec2& size,
               const y::fvec4& panel, const y::fvec4& item,
               const y::fvec4& select)
  : _origin(origin)
  , _size(size)
  , _panel(panel)
  , _item(item)
  , _select(select)
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

void UiList::set_panel(const y::fvec4& panel)
{
  _panel = panel;
}

void UiList::set_item(const y::fvec4& item)
{
  _item = item;
}

void UiList::set_select(const y::fvec4& select)
{
  _select = select;
}

void UiList::draw(RenderUtil& util, const std::vector<y::fvec4>& items,
                  const std::vector<std::string>& source,
                  std::size_t select) const
{
  util.irender_fill(RenderUtil::from_grid(_origin),
                    RenderUtil::from_grid(_size), _panel);

  std::int32_t w = _size[xx];
  std::int32_t h = _size[yy];
  std::int32_t offset = std::max<std::int32_t>(
    0, select >= source.size() - h / 2 - 1 ? source.size() - h :
                                             select - h / 2);

  for (std::int32_t i = 0; i < h; ++i) {
    if (i + offset >= std::int32_t(items.size()) ||
        i + offset >= std::int32_t(source.size())) {
      break;
    }

    std::string s = source[i + offset];
    s = std::int32_t(s.length()) > w ? s.substr(0, w - 3) + "..." : s;
    util.irender_text(s, RenderUtil::from_grid(_origin + y::ivec2{0, i}),
                      items[i + offset]);
  }
}

void UiList::draw(RenderUtil& util,
                  const std::vector<std::string>& source,
                  std::size_t select) const
{
  std::vector<y::fvec4> items;
  for (std::size_t n = 0; n < source.size(); ++n) {
    items.emplace_back(n == select ? _select : _item);
  }

  draw(util, items, source, select);
}

TextInputResult::TextInputResult()
  : success(false)
  , result("")
{
}

TextInputModal::TextInputModal(
    RenderUtil& util, const std::string& default_text,
    TextInputResult& output, const std::string& label)
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
  static const std::string allowed =
      std::string("abcdefghijklmnopqrstuvwxyz") +
      std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ") +
      std::string("0123456789[](){}_-+=@#&./ ");
  if (e.type == sf::Event::TextEntered && e.text.unicode < 128 &&
      allowed.find_first_of(char(e.text.unicode)) != std::string::npos) {
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
    case sf::Keyboard::Home:
      _cursor = 0;
      break;
    case sf::Keyboard::End:
      _cursor = _output.result.size();
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
      std::int32_t(std::max(_label.length(), 1 + _output.result.length())), 2});
  y::ivec2 origin = r.size / 2 - size / 2;
  _util.irender_fill(origin, {size[xx], RenderUtil::from_grid()[yy]},
                     colour::panel);
  _util.irender_outline(origin - y::ivec2{1, 1},
                        y::ivec2{2, 2} + size, colour::outline);
  _util.irender_fill(origin + RenderUtil::from_grid({std::int32_t(_cursor), 1}),
                     RenderUtil::from_grid(), colour::item);
  _util.irender_text(_label, origin, colour::item);
  _util.irender_text(_output.result, origin + RenderUtil::from_grid({0, 1}),
                     colour::select);
  if (_cursor < _output.result.length()) {
    _util.irender_text(_output.result.substr(_cursor, 1),
                       origin + RenderUtil::from_grid({std::int32_t(_cursor), 1}),
                       colour::black);
  }
}

ConfirmationResult::ConfirmationResult()
  : confirm(false)
{
}

ConfirmationModal::ConfirmationModal(
    RenderUtil& util, ConfirmationResult& output, const std::string& message)
  : _util(util)
  , _output(output)
  , _message(message)
{
  _output.confirm = false;
}

void ConfirmationModal::event(const sf::Event& e)
{
  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  switch (e.key.code) {
    case sf::Keyboard::A:
    case sf::Keyboard::Left:
    case sf::Keyboard::Home:
      _output.confirm = true;
      break;
    case sf::Keyboard::D:
    case sf::Keyboard::Right:
    case sf::Keyboard::End:
      _output.confirm = false;
      break;
    case sf::Keyboard::Tab:
      _output.confirm = !_output.confirm;
      break;
    case sf::Keyboard::Return:
      end();
      break;
    case sf::Keyboard::Escape:
      _output.confirm = false;
      end();
      break;
    default: {}
  }
}

void ConfirmationModal::update()
{
}

void ConfirmationModal::draw() const
{
  const Resolution& r = _util.get_window().get_mode();
  y::ivec2 size = RenderUtil::from_grid({
      std::int32_t(std::max(_message.length(), std::size_t(10))), 2});
  y::ivec2 origin = r.size / 2 - size / 2;
  _util.irender_fill(origin, {size[xx], RenderUtil::from_grid()[yy]},
                     colour::panel);
  _util.irender_outline(origin - y::ivec2{1, 1},
                        y::ivec2{2, 2} + size, colour::outline);
  _util.irender_text(_message, origin, colour::item);
  _util.irender_text(_output.confirm ? "[Yes]" : " Yes ",
                     origin + RenderUtil::from_grid({0, 1}),
                     _output.confirm ? colour::select : colour::item);
  _util.irender_text(!_output.confirm ? "[No]" : " No ",
                     origin + y::ivec2{size[xx], 0} +
                         RenderUtil::from_grid({-4, 1}),
                     !_output.confirm ? colour::select : colour::item);
}
