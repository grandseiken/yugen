#ifndef EDITOR_YEDIT_H
#define EDITOR_YEDIT_H

#include "../modal.h"
#include "../ui_util.h"

class Databank;
class Filesystem;
class GlUtil;
class RenderUtil;
class Window;
namespace sf {
  class Event;
}

class Yedit : public Modal {
public:

  Yedit(Filesystem& output, Databank& bank, RenderUtil& util);
  ~Yedit() override {}

  void event(const sf::Event& e) override;
  void update() override;
  void draw() const override;

private:

  Filesystem& _output;
  Databank& _bank;
  RenderUtil& _util;

  UiList _tileset_list;
  UiList _map_list;
  UiList _cell_list;
  UiList _script_list;

  std::int32_t _list_select;
  std::int32_t _tileset_select;
  std::int32_t _map_select;
  std::int32_t _cell_select;
  std::int32_t _script_select;

  TextInputResult _input_result;

};

#endif
