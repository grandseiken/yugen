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

// A default bayer matrix for ordered dithering. Area should be at least the
// ratio of source colours to target colours (per channel); for example 24bpp
// to 9bpp gives 2^8/2^3 = 32 so at least 6x6. This one is 8x8.
const GLfloat bayer_d = 1.f / 65;
const GLfloat bayer_matrix[] = {
    bayer_d * 1, bayer_d * 49, bayer_d * 13, bayer_d * 61,
    bayer_d * 4, bayer_d * 52, bayer_d * 16, bayer_d * 64,
    bayer_d * 33, bayer_d * 17, bayer_d * 45, bayer_d * 29,
    bayer_d * 36, bayer_d * 20, bayer_d * 48, bayer_d * 32,
    bayer_d * 9, bayer_d * 57, bayer_d * 5, bayer_d * 53,
    bayer_d * 12, bayer_d * 60, bayer_d * 8, bayer_d * 56,
    bayer_d * 41, bayer_d * 25, bayer_d * 37, bayer_d * 21,
    bayer_d * 44, bayer_d * 28, bayer_d * 40, bayer_d * 24,
    bayer_d * 3, bayer_d * 51, bayer_d * 15, bayer_d * 63,
    bayer_d * 2, bayer_d * 50, bayer_d * 14, bayer_d * 62,
    bayer_d * 35, bayer_d * 19, bayer_d * 47, bayer_d * 31,
    bayer_d * 34, bayer_d * 18, bayer_d * 46, bayer_d * 30,
    bayer_d * 11, bayer_d * 59, bayer_d * 7, bayer_d * 55,
    bayer_d * 10, bayer_d * 58, bayer_d * 6, bayer_d * 54,
    bayer_d * 43, bayer_d * 27, bayer_d * 39, bayer_d * 23,
    bayer_d * 42, bayer_d * 26, bayer_d * 38, bayer_d * 22};

const GLfloat vertex_data[] = {
    -1.f, -1.f,
     1.f, -1.f,
    -1.f, +1.f,
     1.f, +1.f};

Yugen::Yugen(RenderUtil& util)
  : _util(util)
  , _framebuffer(util.get_gl().make_framebuffer(RenderUtil::native_size))
  , _post_buffer(util.get_gl().make_framebuffer(RenderUtil::native_size))
  , _stage(y::null)
  , _post_program(util.get_gl().make_program({
      "/shaders/post.v.glsl",
      "/shaders/post.f.glsl"}))
  , _upscale_program(util.get_gl().make_program({
      "/shaders/upscale.v.glsl",
      "/shaders/upscale.f.glsl"}))
  , _bayer_texture(util.get_gl().make_texture(
      y::ivec2{8, 8}, GL_FLOAT, GL_R8, GL_RED, bayer_matrix, true))
  , _bayer_frame(0)
  , _vertex_buffer(util.get_gl().make_buffer<GLfloat, 2>(
      GL_ARRAY_BUFFER, GL_STATIC_DRAW, vertex_data, sizeof(vertex_data)))
{
}

const GlFramebuffer& Yugen::get_framebuffer() const
{
  return _framebuffer;
}

void Yugen::set_stage(GameStage* stage)
{
  _stage = stage;
}

Yugen::~Yugen()
{
  _util.get_gl().delete_texture(_bayer_texture);
  _util.get_gl().delete_framebuffer(_framebuffer);
  _util.get_gl().delete_framebuffer(_post_buffer);
  _util.get_gl().delete_program(_post_program);
  _util.get_gl().delete_program(_upscale_program);
  _util.get_gl().delete_buffer(_vertex_buffer);
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
}

void Yugen::draw() const
{
  // TODO: window framerate limiting is untrustworthy. Lock to 60 somehow?
  // And make this clock tick properly. Use std::chrono.
  _measurements.emplace_back(_clock.restart().asMilliseconds());
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

  // Render framebuffer to second buffer with post-processing.
  _post_buffer.bind(true, true);
  _post_program.bind();
  _post_program.bind_attribute("position", _vertex_buffer);
  _post_program.bind_uniform("native_res", _framebuffer.get_size());
  _framebuffer.get_texture().bind(GL_TEXTURE0);
  _post_program.bind_uniform("framebuffer", 0);
  _bayer_texture.bind(GL_TEXTURE1);
  _post_program.bind_uniform("bayer", 1);
  _post_program.bind_uniform("bayer_res", _bayer_texture.get_size());
  _post_program.bind_uniform(
      "bayer_off", _stage ? -y::ivec2(_stage->get_camera()) : y::ivec2());
  _post_program.bind_uniform("bayer_frame", y::int32(++_bayer_frame));
  _util.quad().draw_elements(GL_TRIANGLE_STRIP, 4);

  // Render postbuffer to a file.
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tilde)) {
    y::ivec2 size = _framebuffer.get_size();
    y::int32 length = 4 * size[xx] * size[yy];
    unsigned char* data = new unsigned char[length];
    glReadPixels(0, 0, size[xx], size[yy], GL_RGBA, GL_UNSIGNED_BYTE, data);
    _save_file_frames.emplace_back(data);
  }
  else {
    y::size n = 0;
    for (unsigned char* data : _save_file_frames) {
      y::ivec2 size = _post_buffer.get_size();
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

  // Render FPS indicator.
  if (total) {
    y::sstream ss;
    ss << total << " ticks (" << (1000.f / total) << " fps)";
    _util.render_text(ss.str(), {16, 16}, colour::white);
  }

  // Upscale the native-resolution buffer to the window.
  const Resolution& screen = _util.get_window().get_mode();
  _util.get_gl().bind_window(true, true);
  _upscale_program.bind();
  _upscale_program.bind_attribute("position", _vertex_buffer);
  _upscale_program.bind_uniform("screen_res", screen.size);
  _upscale_program.bind_uniform("native_res", _post_buffer.get_size());
  _upscale_program.bind_uniform("integral_scale_lock", true);
  _post_buffer.get_texture().bind(GL_TEXTURE0);
  _upscale_program.bind_uniform("framebuffer", 0);
  _util.quad().draw_elements(GL_TRIANGLE_STRIP, 4);
}

y::int32 main(y::int32 argc, char** argv)
{
  y::string_vector args;
  for (y::int32 i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  // Usage: yugen map x y
  if (!args.empty() && args.size() != 3) {
    std::cerr << "Usage: yugen map x y" << std::endl;
    return 1;
  }

  Window window("Crunk Yugen", 24, RenderUtil::native_size,
                args.empty(), !args.empty());
  PhysicalFilesystem filesystem("data");
  GlUtil gl(filesystem, window);
  if (!gl) {
    return 1;
  }
  Databank databank(filesystem, gl);
  RenderUtil util(gl);

  y::size map = 0;
  y::wvec2 world;
  if (!args.empty()) {
    if (databank.maps.is_name_used(args[0])) {
      map = databank.maps.get_index(args[0]);
    }
    y::ivec2 tile{std::stoi(args[1]), std::stoi(args[2])};
    world = (y::wvec2{.5, .5} + y::wvec2(tile)) *
             y::wvec2(Tileset::tile_size);
  }

  Yugen* yugen = new Yugen(util);
  GameStage* stage = new GameStage(
      databank, util, yugen->get_framebuffer(),
      databank.maps.get(map), world);
  yugen->set_stage(stage);

  ModalStack stack;
  stack.push(y::move_unique(yugen));
  stack.push(y::move_unique(stage));
  stack.run(window);
  return 0;
}
