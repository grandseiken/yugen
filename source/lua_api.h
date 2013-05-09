// Magic include file doesn't need preprocessor guards. Types and functions
// defined here with the ylib macros are automatically exposed to Lua.
#ifndef LUA_API_H
#define LUA_API_H
#include "render_util.h"
#endif

ylib_typedef(GameStage);
ylib_typedef(Script);

// TODO: integrate y::ivec2 (or fixed) with Lua properly.
ylib_api(get_origin) ylib_arg(const Script*, script)
{
  y::vector<y::int32> origin{script->get_origin()[xx],
                             script->get_origin()[yy]};
  ylib_return(origin);
}

ylib_api(get_region) ylib_arg(const Script*, script)
{
  y::vector<y::int32> region{script->get_region()[xx],
                             script->get_region()[yy]};
  ylib_return(region);
}

ylib_api(set_origin)
    ylib_arg(Script*, script) ylib_arg(y::vector<y::int32>, origin)
{
  script->set_origin({origin[0], origin[1]});
  ylib_void();
}

ylib_api(set_region)
    ylib_arg(Script*, script) ylib_arg(y::vector<y::int32>, region)
{
  script->set_region({region[0], region[1]});
  ylib_void();
}

ylib_api(render_sprite)
    ylib_arg(const Script*, script)
    ylib_arg(y::string, texture)
    ylib_arg(y::int32, frame_width) ylib_arg(y::int32, frame_height)
    ylib_arg(y::int32, frame_x) ylib_arg(y::int32, frame_y)
    ylib_arg(float, depth)
{
  RenderUtil& util = stage.get_util();
  GlTexture t = util.get_gl().get_texture(texture);
  y::ivec2 origin = script->get_origin() - t.get_size() / 2;
  util.render_sprite(t, {frame_width, frame_height}, origin,
                     {frame_x, frame_y}, depth, Colour::white);
  ylib_void();
}
