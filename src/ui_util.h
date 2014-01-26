#ifndef UI_UTIL_H
#define UI_UTIL_H

#include "modal.h"
#include "render/util.h"
#include "vec.h"

// List UI element.
class UiList {
public:

  UiList(const y::ivec2& origin, const y::ivec2& size,
         const y::fvec4& panel, const y::fvec4& item, const y::fvec4& select);

  void set_origin(const y::ivec2& origin);
  void set_size(const y::ivec2& size);

  const y::ivec2& get_origin() const;
  const y::ivec2& get_size() const;

  void set_panel(const y::fvec4& panel);
  void set_item(const y::fvec4& item);
  void set_select(const y::fvec4& select);

  // Draw with colour lookup from list.
  void draw(RenderUtil& util, const y::vector<y::fvec4>& items,
            const y::vector<y::string>& source, y::size select) const;

  // Draw with default colour values.
  void draw(RenderUtil& util,
            const y::vector<y::string>& source, y::size select) const;

private:

  y::ivec2 _origin;
  y::ivec2 _size;
  y::fvec4 _panel;
  y::fvec4 _item;
  y::fvec4 _select;

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
  ~TextInputModal() override {}

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  RenderUtil& _util;
  TextInputResult& _output;
  y::string _label;
  y::size _cursor;

};

struct ConfirmationResult {
  ConfirmationResult();

  bool confirm;
};

class ConfirmationModal : public Modal {
public:

  ConfirmationModal(RenderUtil& util, ConfirmationResult& output,
                    const y::string& message);
  ~ConfirmationModal() override {}

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  RenderUtil& _util;
  ConfirmationResult& _output;
  y::string _message;

};

#endif
