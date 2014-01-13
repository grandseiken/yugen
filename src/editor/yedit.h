#ifndef EDITOR__YEDIT_H
#define EDITOR__YEDIT_H

#include "../common/math.h"
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

  y::int32 _list_select;
  y::int32 _tileset_select;
  y::int32 _map_select;
  y::int32 _cell_select;
  y::int32 _script_select;

  TextInputResult _input_result;

};

#endif
