// Magic include file doesn't need preprocessor guards. Types and functions
// defined here with the ylib macros are automatically exposed to Lua.
#ifndef LUA_API_H
#define LUA_API_H
#include "game_stage.h"
#include "render_util.h"
#endif

ylib_typedef(GameStage);
ylib_typedef(Script);
typedef y::int32 ylib_vec_component;
typedef y::vec<ylib_vec_component, 2> ylib_vec;

/******************************************************************************/
/* Vector API                                                                 */
/******************************************************************************/
ylib_api(vec) ylib_arg(ylib_vec_component, x) ylib_arg(ylib_vec_component, y)
{
  ylib_return(ylib_vec{x, y});
}

ylib_api(vec_add) ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
{
  ylib_return(a + b);
}

ylib_api(vec_sub) ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
{
  ylib_return(a - b);
}

ylib_api(vec_mul) ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
{
  ylib_return(a * b);
}

ylib_api(vec_div) ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
{
  ylib_return(a.euclidean_div(b));
}

ylib_api(vec_mod) ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
{
  ylib_return(a.euclidean_mod(b));
}

ylib_api(vec_unm) ylib_refarg(const ylib_vec, v)
{
  ylib_return(-v);
}

ylib_api(vec_eq) ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
{
  ylib_return(a == b);
}

ylib_api(vec_x) ylib_refarg(const ylib_vec, v)
{
  ylib_return(v[xx]);
}

ylib_api(vec_y) ylib_refarg(const ylib_vec, v)
{
  ylib_return(v[yy]);
}

ylib_api(vec_normalised) ylib_refarg(const ylib_vec, v)
{
  ylib_return(v.normalised());
}

ylib_api(vec_normalise) ylib_refarg(ylib_vec, v)
{
  v.normalise();
  ylib_void();
}

ylib_api(vec_dot) ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
{
  ylib_return(a.dot(b));
}

ylib_api(vec_length_squared) ylib_refarg(const ylib_vec, v)
{
  ylib_return(v.length_squared());
}

ylib_api(vec_length) ylib_refarg(const ylib_vec, v)
{
  ylib_return(v.length());
}

ylib_api(vec_angle)
    ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
{
  ylib_return(a.angle(b));
}

ylib_api(vec_in_region)
    ylib_refarg(const ylib_vec, v)
    ylib_refarg(const ylib_vec, origin) ylib_refarg(const ylib_vec, size)
{
  ylib_return(v.in_region(origin, size));
}

ylib_api(vec_abs) ylib_refarg(const ylib_vec, v)
{
  ylib_return(y::abs(v));
}

ylib_api(vec_max) ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
{
  ylib_return(y::max(a, b));
}

ylib_api(vec_min) ylib_refarg(const ylib_vec, a) ylib_refarg(const ylib_vec, b)
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
  ylib_return(a.valid());
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
    ylib_arg(Script*, script) ylib_refarg(const ylib_vec, origin)
{
  script->set_origin(origin);
  ylib_void();
}

ylib_api(set_region)
    ylib_arg(Script*, script) ylib_refarg(const ylib_vec, region)
{
  script->set_region(region);
  ylib_void();
}

ylib_api(render_sprite)
    ylib_arg(const Script*, script) ylib_arg(y::string, texture)
    ylib_refarg(const ylib_vec, frame_size) ylib_refarg(const ylib_vec, frame)
    ylib_arg(double, depth)
{
  RenderUtil& util = stage.get_util();
  GlTexture t = util.get_gl().get_texture(texture);
  y::ivec2 origin = script->get_origin() - t.get_size() / 2;
  util.render_sprite(t, frame_size, origin, frame, depth, Colour::white);
  ylib_void();
}
