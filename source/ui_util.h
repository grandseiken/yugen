#ifndef UI_UTIL_H
#define UI_UTIL_H

#include "common.h"
#include "modal.h"
#include "render_util.h"
#include "vector.h"

// List UI element.
class UiList {
public:

  UiList(const y::ivec2& origin, const y::ivec2& size,
         const Colour& panel, const Colour& item, const Colour& select);

  void set_origin(const y::ivec2& origin);
  void set_size(const y::ivec2& size);

  const y::ivec2& get_origin() const;
  const y::ivec2& get_size() const;

  void set_panel(const Colour& panel);
  void set_item(const Colour& item);
  void set_select(const Colour& select);

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
  Colour _select;

};

struct TextInputResult {
  TextInputResult();

  bool success;
  y::string result;
};

class TextInputModal : public Modal {
public:

  TextInputModal(RenderUtil& util, const y::string& default_text,
                 TextInputResult& output, const y::string& label);
  virtual ~TextInputModal() {}

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  RenderUtil& _util;
  TextInputResult& _output;
  y::string _label;
  y::size _cursor;

};

#endif
