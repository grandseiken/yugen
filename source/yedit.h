#ifndef YEDIT_H
#define YEDIT_H

#include "common.h"
#include "modal.h"
#include "ui_util.h"

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
  virtual ~Yedit() {}

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

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

};

#endif
