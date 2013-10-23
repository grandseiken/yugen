#include "yugen.h"

#include "databank.h"
#include "game_stage.h"
#include "gl_util.h"
#include "compressed_filesystem.h"
#include "physical_filesystem.h"
#include "render_util.h"
#include "window.h"

#include <iomanip>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace {
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

  // Uses "a dither" algorithm based on http://pippin.gimp.org/a_dither.
  enum a_dither_pattern {
    XOR_PATTERN,
    XOR_PATTERN_RGBSPLIT,
    ADD_PATTERN,
    ADD_PATTERN_RGBSPLIT,
  };

  constexpr y::int32 a_dither_dim(a_dither_pattern pattern)
  {
    return pattern < ADD_PATTERN ? 512 : 256;
  }

  constexpr y::int32 a_dither_size(a_dither_pattern pattern)
  {
    return 3 * a_dither_dim(pattern) * a_dither_dim(pattern);
  }

  GLfloat a_dither_compute(a_dither_pattern pattern, y::int32 n)
  {
    y::size size = a_dither_dim(pattern);
    y::int32 x = n % size;
    y::int32 y = (n / size) % size;
    y::int32 c = n / (size * size);

    switch (pattern) {
      case XOR_PATTERN:
        return ((x ^ y * 149) * 1234 & 511) / 512.f;
      case XOR_PATTERN_RGBSPLIT:
        return (((x + c * 17) ^ y * 149) * 1234 & 511) / 512.f;
      case ADD_PATTERN:
        return ((x + y * 237) * 119 & 255) / 256.f;
      case ADD_PATTERN_RGBSPLIT:
        return (((x + c * 67) + y * 236) * 119 & 255) / 256.f;
      default:
        return .5f;
    }
  }

  const bool use_a_dither = true;
  const a_dither_pattern a_dither = ADD_PATTERN_RGBSPLIT;
  GLfloat a_dither_matrix[a_dither_size(a_dither)];
}

Yugen::Yugen(RenderUtil& util, RunTiming& run_timing)
  : _recording(false)
  , _util(util)
  , _run_timing(run_timing)
  , _framebuffer(util.get_gl().make_unique_framebuffer(
        RenderUtil::native_overflow_size, false, false))
  , _crop_buffer(util.get_gl().make_unique_framebuffer(
        RenderUtil::native_size, false, false))
  , _post_buffer(util.get_gl().make_unique_framebuffer(
        RenderUtil::native_size, false, false))
  , _stage(y::null)
  , _crop_program(util.get_gl().make_unique_program({
      "/shaders/crop.v.glsl",
      "/shaders/crop.f.glsl"}))
  , _post_program(util.get_gl().make_unique_program({
      "/shaders/post.v.glsl",
      "/shaders/post.f.glsl"}))
  , _upscale_program(util.get_gl().make_unique_program({
      "/shaders/upscale.v.glsl",
      "/shaders/upscale.f.glsl"}))
  , _bayer_texture(util.get_gl().make_unique_texture<float, 3>(
      y::ivec3{8, 8, 1}, GL_R8, GL_RED, bayer_matrix, true))
  , _a_dither_texture()
  , _dither_frame(0)
{
  for (y::int32 n = 0; n < a_dither_size(a_dither); ++n) {
    a_dither_matrix[n] = a_dither_compute(a_dither, n);
  }
  _a_dither_texture.swap(util.get_gl().make_unique_texture<float, 3>(
      y::ivec3{a_dither_dim(a_dither), a_dither_dim(a_dither), 3},
      GL_R8, GL_RED, a_dither_matrix, true));
}

const GlFramebuffer& Yugen::get_framebuffer() const
{
  return *_framebuffer;
}

void Yugen::set_stage(GameStage* stage)
{
  _stage = stage;
}

Yugen::~Yugen()
{
  for (unsigned char* data : _save_file_frames) {
    delete[] data;
  }
}

void Yugen::event(const sf::Event& e)
{
  if (e.type != sf::Event::KeyPressed) {
    return;
  }

  if (e.key.code == sf::Keyboard::Escape) {
    end();
  }
}

void Yugen::update()
{
}

void Yugen::draw() const
{
  // Render the game.
  _util.set_resolution(_framebuffer->get_size());
  draw_next();

  // Crop and rotate to the crop buffer.
  _crop_buffer->bind(true, true);
  crop_render(*_framebuffer, _crop_buffer->get_size());

  // Render crop buffer to post buffer with post-processing.
  _post_buffer->bind(true, true);
  post_render(*_crop_buffer, _post_buffer->get_size());

  // Render post buffer to a file.
  recording_render(*_post_buffer);

  // Render debug status.
  _util.set_resolution(_post_buffer->get_size());
  y::sstream ss;
  float fps_avg = 1000000.f / _run_timing.us_per_frame_avg;
  float fps_inst = 1000000.f / _run_timing.us_per_frame_inst;
  float update_pct = 100.f *
      float(_run_timing.us_per_update_avg) / _run_timing.us_per_frame_avg;
  ss << std::setw(5) <<
      _run_timing.us_per_update_avg << " / " <<
      std::setw(5) <<
      _run_timing.us_per_draw_avg << " / " <<
      std::setw(5) <<
      _run_timing.us_per_frame_avg << " avg (" <<
      std::setw(5) << std::setprecision(1) << std::fixed <<
      fps_avg << " fps)";
  _util.irender_text(ss.str(), {16, 16}, colour::white);

  ss.str(y::string());
  ss.clear();
  ss << std::setw(5) <<
      _run_timing.us_per_update_inst << " / " <<
      std::setw(5) <<
      _run_timing.us_per_draw_inst << " / " <<
      std::setw(5) <<
      _run_timing.us_per_frame_inst << " inst (" <<
      std::setw(5) << std::setprecision(1) << std::fixed <<
      fps_inst << " fps)";
  _util.irender_text(ss.str(), {16, 24}, colour::white);

  ss.str(y::string());
  ss.clear();
  ss << std::setw(5) << std::setprecision(1) << std::fixed <<
      update_pct << "% update; " <<
      std::setw(1) <<
      _run_timing.updates_this_cycle << " updates";
  if (_recording) {
    ss << " [recording]";
  }
  _util.irender_text(ss.str(), {16, 32}, colour::white);

  // Upscale the post-buffer to the window.
  const Resolution& screen = _util.get_window().get_mode();
  _util.get_gl().bind_window(true, true);
  upscale_render(*_post_buffer, screen.size);
}

void Yugen::crop_render(const GlFramebuffer& source,
                        const y::ivec2& target_size) const
{
  _crop_program->bind();
  _crop_program->bind_attribute("position", _util.quad_vertex());
  _crop_program->bind_uniform("native_res", target_size);
  _crop_program->bind_uniform("native_overflow_res", source.get_size());
  _crop_program->bind_uniform(
      "rotation", _stage ? float(_stage->get_camera().get_rotation()) : 0.f);
  _crop_program->bind_uniform("framebuffer", source);
  _util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}

void Yugen::post_render(const GlFramebuffer& source,
                        const y::ivec2& target_size) const
{
  const GlUnique<GlTexture3D>& dither = use_a_dither ?
      _a_dither_texture : _bayer_texture;

  _post_program->bind();
  _post_program->bind_attribute("position", _util.quad_vertex());
  _post_program->bind_uniform("native_res", target_size);
  _post_program->bind_uniform("framebuffer", source);
  _post_program->bind_uniform("dither_matrix", *dither);
  _post_program->bind_uniform("dither_res", y::ivec2{
      dither->get_size()[xx], dither->get_size()[yy]});
  _post_program->bind_uniform(
      "dither_off", _stage ?
          y::fvec2(_stage->get_camera().world_to_camera(y::wvec2())) :
          y::fvec2());
  _post_program->bind_uniform(
      "dither_rot", _stage ? float(_stage->get_camera().get_rotation()) : 0.f);
  _post_program->bind_uniform("dither_frame", y::int32(++_dither_frame));
  _util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}

void Yugen::upscale_render(const GlFramebuffer& source,
                           const y::ivec2& target_size) const
{
  _upscale_program->bind();
  _upscale_program->bind_attribute("position", _util.quad_vertex());
  _upscale_program->bind_uniform("screen_res", target_size);
  _upscale_program->bind_uniform("native_res", source.get_size());
  _upscale_program->bind_uniform("integral_scale_lock", true);
  _upscale_program->bind_uniform("use_epx", false);
  _upscale_program->bind_uniform("use_fra", false);
  _upscale_program->bind_uniform("framebuffer", source);
  _util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}

void Yugen::recording_render(const GlFramebuffer& source) const
{
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tilde)) {
    _recording = true;
  }
  if (_recording) {
    y::ivec2 size = source.get_size();
    y::int32 length = 4 * size[xx] * size[yy];
    unsigned char* data = new unsigned char[length];
    glReadPixels(0, 0, size[xx], size[yy], GL_RGBA, GL_UNSIGNED_BYTE, data);
    _save_file_frames.emplace_back(data);
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::BackSpace)) {
    _recording = false;
    _save_file_frames.clear();
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Return)) {
    _recording = false;
    y::size n = 0;
    for (unsigned char* data : _save_file_frames) {
      y::ivec2 size = source.get_size();
      sf::Image image;
      image.create(size[xx], size[yy], data);
      image.flipVertically();

      y::sstream ss;
      ss << "tmp/" << std::setw(4) << std::setfill('0') << n++ << ".png";
      image.saveToFile(ss.str());
      delete[] data;
      std::cout << "Saved frame " << n << " of " << _save_file_frames.size() <<
          std::endl;
    }
    _save_file_frames.clear();
  }
  _run_timing.target_updates_per_second =
      _run_timing.target_draws_per_second = _recording ? 0.f : 60.f;
}

y::int32 main(y::int32 argc, char** argv)
{
  y::string_vector args;
  for (y::int32 i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  // Usage: yugen [map x y]
  if (!args.empty() && args.size() != 3) {
    std::cerr << "Usage: yugen [map x y]" << std::endl;
    return 1;
  }

  Window window("Crunk Yugen", 24, RenderUtil::native_size,
                args.empty(), !args.empty());
  PhysicalFilesystem data_filesystem("data");
  GlUtil gl(data_filesystem, window);
  if (!gl) {
    return 1;
  }
  Databank databank(data_filesystem, gl);
  RenderUtil util(gl);

  y::string map = databank.maps.get_names()[0];
  y::wvec2 world;
  if (!args.empty()) {
    if (databank.maps.is_name_used(args[0])) {
      map = args[0];
    }
    y::ivec2 tile{std::stoi(args[1]), std::stoi(args[2])};
    world = (y::wvec2{.5, .5} + y::wvec2(tile)) *
             y::wvec2(Tileset::tile_size);
  }

  RunTiming run_timing;
  run_timing.target_updates_per_second = 60.f;
  run_timing.target_draws_per_second = 60.f;
  Yugen* yugen = new Yugen(util, run_timing);

  PhysicalFilesystem save_filesystem_temp("save");
  CompressedFilesystem save_filesystem(
      save_filesystem_temp, CompressedFilesystem::BZIP2);
  save_filesystem.add_compressed_extension("sav");

  GameStage* stage = new GameStage(
      databank, save_filesystem,
      util, yugen->get_framebuffer(), map, world);
  yugen->set_stage(stage);

  ModalStack stack;
  stack.push(y::move_unique(yugen));
  stack.push(y::move_unique(stage));
  stack.run(window, run_timing);
  return 0;
}
