#include "databank.h"
#include "gl_util.h"
#include "modal.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "window.h"

#include <SFML/Window.hpp>

class Editor : public Modal {
public:

  Editor(Window& window, GlUtil& gl, RenderUtil& util);
  virtual ~Editor() {}

  virtual void event(const sf::Event& e);
  virtual void update();
  virtual void draw() const;

private:

  Window& _window;
  GlUtil& _gl;
  RenderUtil& _util;

};

int main(int argc, char** argv)
{
  Window window("Crunk Yedit", 24, 0, 0, false, true);
  PhysicalFilesystem filesystem("data");
  GlUtil gl(filesystem, window);
  if (!gl) {
    return 1;
  }
  Databank databank(filesystem, gl);
  RenderUtil util(gl);
  
  ModalStack stack;
  stack.push(y::move_unique(new Editor(window, gl, util)));
  stack.run(window);
  return 0;
}

Editor::Editor(Window& window, GlUtil& gl, RenderUtil& util)
  : _window(window)
  , _gl(gl)
  , _util(util)
{
}

void Editor::event(const sf::Event& e)
{
  if (e.type == sf::Event::Closed ||
      (e.type == sf::Event::KeyPressed &&
       e.key.code == sf::Keyboard::Escape)) {
    end();
  }
}

void Editor::update()
{
}

void Editor::draw() const
{
  _gl.bind_window();
  const Resolution& screen = _window.get_mode();
  _util.set_resolution(screen.width, screen.height);
  _util.render_text("Hello, world!", 16.f, 16.f,
                    1.f, 1.f, 1.f, 1.f);
  _util.render_text("HHHHHHHHHHHH", 8.f, 8.f,
                    1.f, 0.f, 0.f, 1.f);
  _util.render_text("Hdawkfhawkfj", 0.f, 0.f,
                    1.f, 1.f, 1.f, 1.f);
}
