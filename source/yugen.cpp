#include "yugen.h"

#include "databank.h"
#include "game_stage.h"
#include "gl_util.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "window.h"

#include <iomanip>
#include <SFML/Graphics.hpp>
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
  , _launched(false)
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
  for (unsigned char* data : _save_file_frames) {
    delete[] data;
  }
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
  // Start a new game.
  if (!has_next() && !_bank.maps.empty() && !_launched) {
    _launched = true;
    push(y::move_unique(new GameStage(
        _util, _framebuffer, _bank.maps.get(0), y::ivec2())));
  }
}

void Yugen::draw() const
{
  // TODO: window framerate limiting is untrustworthy. Lock to 60 somehow?
  // And make this clock tick properly.
  _measurements.push_back(_clock.restart().asMilliseconds());
  if (_measurements.size() > samples) {
    _measurements.erase(_measurements.begin());
  }
  y::size total = 0;
  if (!_measurements.empty()) {
    for (y::size m : _measurements) {
      total += m;
    }
    total /= samples;
  }

  // Render the game.
  _framebuffer.bind(true, true);
  _util.set_resolution(_framebuffer.get_size());
  draw_next();

  // Render FPS indicator.
  if (total) {
    y::sstream ss;
    ss << total << " ticks (" << (1000.f / total) << " fps)";
    _util.render_text(ss.str(), {16, 16}, Colour::white);
  }

  // Render framebuffer to a file.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tilde)) {
    y::ivec2 size = _framebuffer.get_size();
    y::int32 length = 4 * size[xx] * size[yy];
    unsigned char* data = new unsigned char[length];
    glReadPixels(0, 0, size[xx], size[yy], GL_RGBA, GL_UNSIGNED_BYTE, data);
    _save_file_frames.push_back(data);
  }
  else {
    y::size n = 0;
    for (unsigned char* data : _save_file_frames) {
      y::ivec2 size = _framebuffer.get_size();
      sf::Image image;
      image.create(size[xx], size[yy], data);
      image.flipVertically();

      y::sstream ss;
      ss << "tmp/" << std::setw(4) << std::setfill('0') << n++ << ".png";
      image.saveToFile(ss.str());
      delete[] data;
    }
    _save_file_frames.clear();
  }

  // Render framebuffer to screen with post-processing.
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
