// Magic include file doesn't need preprocessor guards. Types and functions
// defined here with the ylib macros are automatically exposed to Lua.
#ifndef LUA_API_H
#define LUA_API_H
#include "render_util.h"
#endif

ylib_api(render_sprite)
    ylib_arg(y::string, texture)
    ylib_arg(y::int32, frame_width) ylib_arg(y::int32, frame_height)
    ylib_arg(y::int32, origin_x) ylib_arg(y::int32, origin_y)
    ylib_arg(y::int32, frame_x) ylib_arg(y::int32, frame_y)
    ylib_arg(float, depth)
{
  RenderUtil& util = stage.get_util();
  util.render_sprite(util.get_gl().get_texture(texture),
                     {frame_width, frame_height}, {origin_x, origin_y},
                     {frame_x, frame_y}, depth, Colour::white);
  ylib_void();
}
