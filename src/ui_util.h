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
  void draw(RenderUtil& util, const std::vector<y::fvec4>& items,
            const std::vector<std::string>& source, std::size_t select) const;

  // Draw with default colour values.
  void draw(RenderUtil& util,
            const std::vector<std::string>& source, std::size_t select) const;

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
  std::string result;
};

class TextInputModal : public Modal {
public:

  TextInputModal(RenderUtil& util, const std::string& default_text,
                 TextInputResult& output, const std::string& label);
  ~TextInputModal() override {}

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  RenderUtil& _util;
  TextInputResult& _output;
  std::string _label;
  std::size_t _cursor;

};

struct ConfirmationResult {
  ConfirmationResult();

  bool confirm;
};

class ConfirmationModal : public Modal {
public:

  ConfirmationModal(RenderUtil& util, ConfirmationResult& output,
                    const std::string& message);
  ~ConfirmationModal() override {}

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  RenderUtil& _util;
  ConfirmationResult& _output;
  std::string _message;

};

#endif
