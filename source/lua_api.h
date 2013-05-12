// Magic include file doesn't need preprocessor guards. Types and functions
// defined here with the ylib macros are automatically exposed to Lua.
#ifndef LUA_API_H
#define LUA_API_H
#include "databank.h"
#include "game_stage.h"
#include "render_util.h"
#endif

ylib_typedef(GameStage);
ylib_typedef(GlTexture);
ylib_typedef(LuaFile);
ylib_typedef(Script);

/******************************************************************************/
/* Vector API                                                                 */
/******************************************************************************/
ylib_api(vec) ylib_arg(y::world, x) ylib_arg(y::world, y)
{
  ylib_return(y::wvec2{x, y});
}

ylib_api(vec_add) ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a + b);
}

ylib_api(vec_sub) ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a - b);
}

ylib_api(vec_mul) ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a * b);
}

ylib_api(vec_div) ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a.euclidean_div(b));
}

ylib_api(vec_mod) ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(y::wvec2{fmod(a[xx], b[xx]), fmod(a[yy], b[yy])});
}

ylib_api(vec_unm) ylib_refarg(const y::wvec2, v)
{
  ylib_return(-v);
}

ylib_api(vec_eq) ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a == b);
}

ylib_api(vec_x) ylib_refarg(const y::wvec2, v)
{
  ylib_return(v[xx]);
}

ylib_api(vec_y) ylib_refarg(const y::wvec2, v)
{
  ylib_return(v[yy]);
}

ylib_api(vec_normalised) ylib_refarg(const y::wvec2, v)
{
  ylib_return(v.normalised());
}

ylib_api(vec_normalise) ylib_refarg(y::wvec2, v)
{
  v.normalise();
  ylib_void();
}

ylib_api(vec_dot) ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a.dot(b));
}

ylib_api(vec_length_squared) ylib_refarg(const y::wvec2, v)
{
  ylib_return(v.length_squared());
}

ylib_api(vec_length) ylib_refarg(const y::wvec2, v)
{
  ylib_return(v.length());
}

ylib_api(vec_angle)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a.angle(b));
}

ylib_api(vec_in_region)
    ylib_refarg(const y::wvec2, v)
    ylib_refarg(const y::wvec2, origin) ylib_refarg(const y::wvec2, size)
{
  ylib_return(v.in_region(origin, size));
}

ylib_api(vec_abs) ylib_refarg(const y::wvec2, v)
{
  ylib_return(y::abs(v));
}

ylib_api(vec_max) ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(y::max(a, b));
}

ylib_api(vec_min) ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(y::min(a, b));
}

/******************************************************************************/
/* Script reference API                                                       */
/******************************************************************************/
ylib_api(ref) ylib_arg(Script*, script)
{
  ylib_return(ScriptReference(*script));
}

ylib_api(ref_eq)
    ylib_refarg(const ScriptReference, a) ylib_refarg(const ScriptReference, b)
{
  ylib_return(a.get() == b.get());
}

ylib_api(ref_gc) ylib_refarg(const ScriptReference, a)
{
  a.~ScriptReference();
  ylib_void();
}

ylib_api(ref_valid) ylib_refarg(const ScriptReference, a)
{
  ylib_return(a.is_valid());
}

ylib_api(ref_get) ylib_refarg(ScriptReference, a)
{
  ylib_return(a.get());
}

/******************************************************************************/
/* Script API                                                                 */
/******************************************************************************/
ylib_api(get_origin) ylib_arg(const Script*, script)
{
  ylib_return(script->get_origin());
}

ylib_api(get_region) ylib_arg(const Script*, script)
{
  ylib_return(script->get_region());
}

ylib_api(set_origin)
    ylib_arg(Script*, script) ylib_refarg(const y::wvec2, origin)
{
  script->set_origin(origin);
  ylib_void();
}

ylib_api(set_region)
    ylib_arg(Script*, script) ylib_refarg(const y::wvec2, region)
{
  script->set_region(region);
  ylib_void();
}

ylib_api(get_sprite) ylib_arg(y::string, path)
{
  ylib_return(&stage.get_bank().sprites.get(path));
}

ylib_api(get_script) ylib_arg(y::string, path)
{
  ylib_return(&stage.get_bank().scripts.get(path));
}

ylib_api(destroy) ylib_arg(Script*, script)
{
  script->destroy();
  ylib_void();
}

ylib_api(create)
    ylib_arg(const LuaFile*, file) ylib_refarg(const y::wvec2, origin)
{
  ylib_return(&stage.create_script(*file, origin));
}

ylib_api(create_region)
    ylib_arg(const LuaFile*, file)
    ylib_refarg(const y::wvec2, origin) ylib_refarg(const y::wvec2, region)
{
  ylib_return(&stage.create_script(*file, origin, region));
}

ylib_api(render_sprite)
    ylib_arg(const Script*, script) ylib_arg(const GlTexture*, sprite)
    ylib_refarg(const y::wvec2, frame_size) ylib_refarg(const y::wvec2, frame)
    ylib_arg(double, depth)
{
  RenderBatch& batch = stage.get_current_batch();
  y::wvec2 origin = script->get_origin() - frame_size / 2 + y::wvec2{.5, .5};
  batch.add_sprite(*sprite, y::ivec2(frame_size), y::ivec2(origin),
                   y::ivec2(frame), depth, colour::white);
  ylib_void();
}
