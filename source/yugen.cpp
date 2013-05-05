#include "yugen.h"

#include "databank.h"
#include "gl_util.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "window.h"

#include <SFML/Window.hpp>

const GLfloat vertex_data[] = {
    -1.f, -1.f,
     1.f, -1.f,
    -1.f, +1.f,
     1.f, +1.f};

Yugen::Yugen(Databank& bank, RenderUtil& util)
  : _bank(bank)
  , _util(util)
  , _framebuffer(util.get_gl().make_framebuffer(RenderUtil::native_size))
  , _post_program(util.get_gl().make_program({
      "/shaders/post.v.glsl",
      "/shaders/post.f.glsl"}))
  , _vertex_buffer(util.get_gl().make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertex_data, sizeof(vertex_data)))
{
}

Yugen::~Yugen()
{
  _util.get_gl().delete_framebuffer(_framebuffer);
}

void Yugen::event(const sf::Event& e)
{
  if (e.type == sf::Event::KeyPressed &&
      e.key.code == sf::Keyboard::Escape) {
    end();
  }
}

void Yugen::update()
{
  // TODO: window framerate limiting is untrustworthy. Lock to 60 somehow?
  _measurements.push_back(_clock.restart().asMilliseconds());
  if (_measurements.size() > samples) {
    _measurements.erase(_measurements.begin());
  }
}

void Yugen::draw() const
{
  y::size total = 0;
  if (!_measurements.empty()) {
    for (y::size m : _measurements) {
      total += m;
    }
    total /= samples;
  }

  _framebuffer.bind(true, true);
  _util.set_resolution(_framebuffer.get_size());
  if (total) {
    y::sstream ss;
    ss << total << " ticks (" << (1000.f / total) << " fps)";
    _util.render_text(ss.str(), {16, 16}, Colour::white);
  }

  const Resolution& screen = _util.get_window().get_mode();
  _util.get_gl().bind_window(true, true);
  _post_program.bind();
  _post_program.bind_attribute("position", _vertex_buffer);
  _post_program.bind_uniform("integral_scale_lock", true);
  _post_program.bind_uniform("native_res", _framebuffer.get_size());
  _post_program.bind_uniform("screen_res", screen.size);
  _framebuffer.get_texture().bind(GL_TEXTURE0);
  _post_program.bind_uniform("framebuffer", 0);
  _util.quad().draw_elements(GL_TRIANGLE_STRIP, 4);
}

y::int32 main(y::int32 argc, char** argv)
{
  (void)argc;
  (void)argv;

  Window window("Crunk Yugen", 24, RenderUtil::native_size, true, false);
  PhysicalFilesystem filesystem("data");
  GlUtil gl(filesystem, window);
  if (!gl) {
    return 1;
  }
  Databank databank(filesystem, gl);
  RenderUtil util(gl);

  ModalStack stack;
  stack.push(y::move_unique(new Yugen(databank, util)));
  stack.run(window);
  return 0;
}
