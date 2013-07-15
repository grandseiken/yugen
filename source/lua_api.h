// Magic include file doesn't need preprocessor guards. Types and functions
// defined here with the ylib macros are automatically exposed to Lua.
#ifndef LUA_API_H
#define LUA_API_H
#include "collision.h"
#include "databank.h"
#include "game_stage.h"
#include "render_util.h"
#endif

ylib_ptrtypedef(Body);
ylib_ptrtypedef(GameStage);
ylib_ptrtypedef(Sprite);
ylib_ptrtypedef(Light);
ylib_ptrtypedef(LuaFile);
ylib_ptrtypedef(Script);

/******************************************************************************/
// Vector API
/******************************************************************************/
ylib_api(vec)
    ylib_arg(y::world, x) ylib_arg(y::world, y)
{
  ylib_return(y::wvec2{x, y});
}

ylib_api(vec_add)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(a + b);
}

ylib_api(vec_sub)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(a - b);
}

ylib_api(vec_mul)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(a * b);
}

ylib_api(vec_div)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(a.euclidean_div(b));
}

ylib_api(vec_mod)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(y::wvec2{fmod(a[xx], b[xx]), fmod(a[yy], b[yy])});
}

ylib_api(vec_unm)
    ylib_arg(const y::wvec2, v)
{
  ylib_return(-v);
}

ylib_api(vec_eq)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(a == b);
}

ylib_api(vec_x)
    ylib_arg(const y::wvec2, v)
{
  ylib_return(v[xx]);
}

ylib_api(vec_y)
    ylib_arg(const y::wvec2, v)
{
  ylib_return(v[yy]);
}

ylib_api(vec_normalised)
    ylib_arg(const y::wvec2, v)
{
  ylib_return(v.normalised());
}

ylib_api(vec_normalise)
    ylib_arg(y::wvec2, v)
{
  v.normalise();
  ylib_void();
}

ylib_api(vec_dot)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(a.dot(b));
}

ylib_api(vec_length_squared)
    ylib_arg(const y::wvec2, v)
{
  ylib_return(v.length_squared());
}

ylib_api(vec_length)
    ylib_arg(const y::wvec2, v)
{
  ylib_return(v.length());
}

ylib_api(vec_angle)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(a.angle(b));
}

ylib_api(vec_in_region)
    ylib_arg(const y::wvec2, v)
    ylib_arg(const y::wvec2, origin) ylib_arg(const y::wvec2, size)
{
  ylib_return(v.in_region(origin, size));
}

ylib_api(vec_abs)
    ylib_arg(const y::wvec2, v)
{
  ylib_return(y::abs(v));
}

ylib_api(vec_max)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(y::max(a, b));
}

ylib_api(vec_min)
    ylib_arg(const y::wvec2, a) ylib_arg(const y::wvec2, b)
{
  ylib_return(y::min(a, b));
}

typedef y::wvec2 ylib_vec;
ylib_valtypedef(ylib_vec)
  ylib_method("__add", vec_add)
  ylib_method("__sub", vec_sub)
  ylib_method("__mul", vec_mul)
  ylib_method("__div", vec_div)
  ylib_method("__mod", vec_mod)
  ylib_method("__unm", vec_unm)
  ylib_method("__eq", vec_eq)
  ylib_method("x", vec_x)
  ylib_method("y", vec_y)
  ylib_method("normalised", vec_normalised)
  ylib_method("normalise", vec_normalise)
  ylib_method("dot", vec_dot)
  ylib_method("length_squared", vec_length_squared)
  ylib_method("length", vec_length)
  ylib_method("angle", vec_angle)
  ylib_method("in_region", vec_in_region)
  ylib_method("abs", vec_abs)
  ylib_method("max", vec_max)
  ylib_method("min", vec_min)
ylib_endtypedef();

/******************************************************************************/
// Script reference API
/******************************************************************************/
ylib_api(ref)
    ylib_arg(Script*, script)
{
  ylib_return(ScriptReference(*script));
}

ylib_api(ref_eq)
    ylib_arg(const ScriptReference, a) ylib_arg(const ScriptReference, b)
{
  ylib_return(a.get() == b.get());
}

ylib_api(ref_gc)
    ylib_arg(const ScriptReference, a)
{
  a.~ScriptReference();
  ylib_void();
}

ylib_api(ref_valid)
    ylib_arg(const ScriptReference, a)
{
  ylib_return(a.is_valid());
}

ylib_api(ref_get)
    ylib_arg(ScriptReference, a)
{
  ylib_return(a.get());
}

ylib_valtypedef(ScriptReference)
  ylib_method("__eq", ref_eq)
  ylib_method("__gc", ref_gc)
  ylib_method("valid", ref_valid)
  ylib_method("get", ref_get)
ylib_endtypedef();

/******************************************************************************/
// Script API
/******************************************************************************/
ylib_api(get_uid)
    ylib_arg(const Script*, script)
{
  ylib_return(stage.get_scripts().get_uid(script));
}

ylib_api(get_origin)
    ylib_arg(const Script*, script)
{
  ylib_return(script->get_origin());
}

ylib_api(get_region)
    ylib_arg(const Script*, script)
{
  ylib_return(script->get_region());
}

ylib_api(get_rotation)
    ylib_arg(const Script*, script)
{
  ylib_return(script->get_rotation());
}

ylib_api(set_origin)
    ylib_arg(Script*, script) ylib_arg(const y::wvec2, origin)
{
  script->set_origin(origin);
  ylib_void();
}

ylib_api(set_region)
    ylib_arg(Script*, script) ylib_arg(const y::wvec2, region)
{
  script->set_region(region);
  ylib_void();
}

ylib_api(set_rotation)
    ylib_arg(Script*, script) ylib_arg(y::world, rotation)
{
  script->set_rotation(rotation);
  ylib_void();
}

ylib_api(get_script)
    ylib_arg(y::string, path)
{
  ylib_return(&stage.get_bank().scripts.get(path));
}

ylib_api(destroy)
    ylib_arg(Script*, script)
{
  script->destroy();
  ylib_void();
}

ylib_api(create)
    ylib_arg(const LuaFile*, file) ylib_arg(const y::wvec2, origin)
{
  ylib_return(&stage.get_scripts().create_script(*file, origin));
}

ylib_api(create_region)
    ylib_arg(const LuaFile*, file)
    ylib_arg(const y::wvec2, origin) ylib_arg(const y::wvec2, region)
{
  ylib_return(&stage.get_scripts().create_script(*file, origin, region));
}

ylib_api(send_message)
    ylib_arg(Script*, script) ylib_arg(y::string, function_name)
    ylib_arg(y::vector<LuaValue>, args)
{
  stage.get_scripts().send_message(script, function_name, args);
  ylib_void();
}

/******************************************************************************/
// Stage API
/******************************************************************************/
ylib_api(get_scripts_in_region)
    ylib_arg(const y::wvec2, origin) ylib_arg(const y::wvec2, region)
{
  ScriptBank::result result;
  stage.get_scripts().get_in_region(result, origin, region);
  ylib_return(result);
}

ylib_api(get_scripts_in_radius)
    ylib_arg(const y::wvec2, origin) ylib_arg(y::world, radius)
{
  ScriptBank::result result;
  stage.get_scripts().get_in_radius(result, origin, radius);
  ylib_return(result);
}

/******************************************************************************/
// Player API
/******************************************************************************/
ylib_api(set_player)
    ylib_arg(Script*, script)
{
  stage.set_player(script);
  ylib_void();
}

ylib_api(clear_player)
{
  stage.set_player(y::null);
  ylib_void();
}

ylib_api(has_player)
{
  ylib_return(stage.get_player() != y::null);
}

ylib_api(get_player)
{
  ylib_return(stage.get_player());
}

ylib_api(is_key_down)
    ylib_arg(y::int32, key)
{
  ylib_return(stage.is_key_down(key));
}

ylib_api(set_camera)
    ylib_arg(const y::wvec2, camera)
{
  stage.get_camera().set_origin(camera);
  ylib_void();
}

ylib_api(get_camera)
{
  ylib_return(stage.get_camera().get_origin());
}

ylib_api(set_camera_rotation)
    ylib_arg(y::world, rotation)
{
  stage.get_camera().set_rotation(rotation);
  ylib_void();
}

ylib_api(get_camera_rotation)
{
  ylib_return(stage.get_camera().get_rotation());
}

/******************************************************************************/
// Rendering API
/******************************************************************************/
ylib_api(get_sprite)
    ylib_arg(y::string, path)
{
  ylib_return(&stage.get_bank().sprites.get(path));
}

ylib_api(render_sprite)
    ylib_arg(const Script*, script) ylib_arg(y::int32, layer)
    ylib_arg(const Sprite*, sprite)
    ylib_arg(const y::wvec2, frame_size) ylib_arg(const y::wvec2, frame)
    ylib_arg(y::world, depth) ylib_arg(y::world, rotation)
    ylib_arg(y::world, r) ylib_arg(y::world, g) ylib_arg(y::world, b)
    ylib_arg(y::world, a)
{
  const GameRenderer& renderer = stage.get_renderer();
  bool normal = renderer.draw_stage_is_normal();

  RenderBatch& batch = renderer.get_current_batch();
  const GlTexture2D& texture = normal ? sprite->normal : sprite->texture;
  y::wvec2 origin = script->get_origin() - frame_size / 2;

  if (renderer.draw_stage_is_layer(GameRenderer::draw_layer(layer))) {
    renderer.set_current_draw_any();
    batch.add_sprite(texture, y::ivec2(frame_size), normal,
                     y::fvec2(origin), y::ivec2(frame),
                     depth, rotation,
                     y::fvec4{float(r), float(g), float(b), float(a)});
  }
  ylib_void();
}

ylib_api(render_fog)
    ylib_arg(y::int32, layer) ylib_arg(y::world, layering_value)
    ylib_arg(const y::wvec2, origin) ylib_arg(const y::wvec2, region)
    ylib_arg(const y::wvec2, tex_offset) ylib_arg(y::world, frame)
    ylib_arg(y::world, r) ylib_arg(y::world, g) ylib_arg(y::world, b)
    ylib_arg(y::world, a)
    ylib_arg(y::world, fog_min) ylib_arg(y::world, fog_max)
{
  const GameRenderer& renderer = stage.get_renderer();
  Environment::fog_params params;
  params.layering_value = layering_value;
  params.tex_offset = tex_offset;
  params.frame = frame;
  params.colour = y::fvec4{float(r), float(g), float(b), float(a)};
  params.fog_min = fog_min;
  params.fog_max = fog_max;

  if (renderer.draw_stage_is_layer(GameRenderer::draw_layer(layer))) {
    renderer.set_current_draw_any();
    if (renderer.draw_stage_is_normal()) {
      stage.get_environment().render_fog_normal(
          renderer.get_util(), origin, region, params);
    }
    else {
      stage.get_environment().render_fog_colour(
          renderer.get_util(), origin, region, params);
    }
  }
  ylib_void();
}

ylib_api(render_reflect)
    ylib_arg(y::int32, layer) ylib_arg(y::world, layering_value)
    ylib_arg(const y::wvec2, origin) ylib_arg(const y::wvec2, region)
    ylib_arg(const y::wvec2, tex_offset) ylib_arg(y::world, frame)
    ylib_arg(y::world, r) ylib_arg(y::world, g) ylib_arg(y::world, b)
    ylib_arg(y::world, a) ylib_arg(y::world, reflect_mix)
    ylib_arg(y::world, normal_scaling)
    ylib_arg(y::world, normal_scaling_reflect)
    ylib_arg(y::world, normal_scaling_refract)
    ylib_arg(y::world, reflect_fade_start) ylib_arg(y::world, reflect_fade_end)
    ylib_arg(bool, flip_x) ylib_arg(bool, flip_y)
    ylib_arg(const y::wvec2, flip_axes)
    ylib_arg(y::world, wave_height) ylib_arg(y::world, wave_scale)
{
  const GameRenderer& renderer = stage.get_renderer();
  Environment::reflect_params params;
  params.layering_value = layering_value;
  params.tex_offset = tex_offset;
  params.frame = frame;
  params.colour = y::fvec4{float(r), float(g), float(b), float(a)};
  params.reflect_mix = reflect_mix;
  params.normal_scaling = normal_scaling;
  params.normal_scaling_reflect = normal_scaling_reflect;
  params.normal_scaling_refract = normal_scaling_refract;
  params.reflect_fade_start = reflect_fade_start;
  params.reflect_fade_end = reflect_fade_end;
  params.flip_x = flip_x;
  params.flip_y = flip_y;
  params.flip_axes = flip_axes;
  params.wave_height = wave_height;
  params.wave_scale = wave_scale;

  if (renderer.draw_stage_is_layer(GameRenderer::draw_layer(layer))) {
    renderer.set_current_draw_any();
    if (renderer.draw_stage_is_normal()) {
      stage.get_environment().render_reflect_normal(
          renderer.get_util(), origin, region, params);
    }
    else {
      stage.get_environment().render_reflect_colour(
          renderer.get_util(), origin, region, params,
          renderer.get_framebuffer());
    }
  }
  ylib_void();
}

/******************************************************************************/
// Collision API
/******************************************************************************/
ylib_api(create_body)
    ylib_arg(Script*, script)
    ylib_arg(const y::wvec2, offset) ylib_arg(const y::wvec2, size)
{
  Body* body = stage.get_collision().create_obj(*script);
  body->offset = offset;
  body->size = size;
  ylib_return(body);
}

ylib_api(destroy_body)
    ylib_arg(const Script*, script) ylib_arg(Body*, body)
{
  stage.get_collision().destroy_obj(*script, body);
  ylib_void();
}

ylib_api(destroy_bodies)
    ylib_arg(const Script*, script)
{
  stage.get_collision().destroy_all(*script);
  ylib_void();
}

ylib_api(get_body_offset)
    ylib_arg(const Body*, body)
{
  ylib_return(body->offset);
}

ylib_api(get_body_size)
    ylib_arg(const Body*, body)
{
  ylib_return(body->size);
}

ylib_api(get_collide_mask)
    ylib_arg(const Body*, body)
{
  ylib_return(body->collide_mask);
}

ylib_api(set_body_offset)
    ylib_arg(Body*, body) ylib_arg(const y::wvec2, offset)
{
  body->offset = offset;
  ylib_void();
}

ylib_api(set_body_size)
    ylib_arg(Body*, body) ylib_arg(const y::wvec2, size)
{
  body->size = size;
  ylib_void();
}

ylib_api(set_collide_mask)
    ylib_arg(Body*, body) ylib_arg(y::int32, collide_mask)
{
  body->collide_mask = collide_mask;
  ylib_void();
}

ylib_api(collider_move)
    ylib_arg(Script*, script) ylib_arg(const y::wvec2, move)
{
  ylib_return(stage.get_collision().collider_move(*script, move));
}

ylib_api(collider_rotate)
   ylib_arg(Script*, script) ylib_arg(y::world, rotate)
{
  ylib_return(stage.get_collision().collider_rotate(*script, rotate));
}

ylib_api(body_check)
    ylib_arg(const Script*, script) ylib_arg(const Body*, body)
    ylib_arg(y::int32, collide_mask)
{
  ylib_return(stage.get_collision().body_check(*script, *body, collide_mask));
}

/******************************************************************************/
// Lighting API
/******************************************************************************/
ylib_api(create_light)
    ylib_arg(Script*, script)
    ylib_arg(y::world, full_range) ylib_arg(y::world, falloff_range)
{
  Light* light = stage.get_lighting().create_obj(*script);
  light->full_range = full_range;
  light->falloff_range = falloff_range;
  light->layer_value = 0.;
  ylib_return(light);
}

ylib_api(destroy_light)
    ylib_arg(const Script*, script) ylib_arg(Light*, light)
{
  stage.get_lighting().destroy_obj(*script, light);
  ylib_void();
}

ylib_api(destroy_lights)
    ylib_arg(const Script*, script)
{
  stage.get_lighting().destroy_all(*script);
  ylib_void();
}

ylib_api(get_light_full_range)
    ylib_arg(const Light*, light)
{
  ylib_return(light->full_range);
}

ylib_api(set_light_range)
    ylib_arg(Light*, light) ylib_arg(y::world, full_range)
{
  light->full_range = full_range;
  ylib_void();
}

ylib_api(get_light_falloff_range)
    ylib_arg(const Light*, light)
{
  ylib_return(light->falloff_range);
}

ylib_api(set_light_falloff_range)
    ylib_arg(Light*, light) ylib_arg(y::world, falloff_range)
{
  light->falloff_range = falloff_range;
  ylib_void();
}

ylib_api(get_light_layer_value)
    ylib_arg(const Light*, light)
{
  ylib_return(light->layer_value);
}

ylib_api(set_light_layer_value)
    ylib_arg(Light*, light) ylib_arg(y::world, layer_value)
{
  light->layer_value = layer_value;
  ylib_void();
}

ylib_api(get_light_r)
    ylib_arg(const Light*, light)
{
  ylib_return(y::world(light->colour[rr]));
}

ylib_api(get_light_g)
    ylib_arg(const Light*, light)
{
  ylib_return(y::world(light->colour[gg]));
}

ylib_api(get_light_b)
    ylib_arg(const Light*, light)
{
  ylib_return(y::world(light->colour[bb]));
}

ylib_api(get_light_intensity)
    ylib_arg(const Light*, light)
{
  ylib_return(y::world(light->colour[aa]));
}

ylib_api(set_light_colour)
    ylib_arg(Light*, light)
    ylib_arg(y::world, r) ylib_arg(y::world, g) ylib_arg(y::world, b)
{
  light->colour[rr] = r;
  light->colour[gg] = g;
  light->colour[bb] = b;
  ylib_void();
}

ylib_api(set_light_intensity)
    ylib_arg(Light*, light) ylib_arg(y::world, intensity)
{
  light->colour[aa] = intensity;
  ylib_void();
}
