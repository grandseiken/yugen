// Magic include file doesn't need preprocessor guards. Types and functions
// defined here with the y macros are automatically exposed to Lua.
#ifndef LUA_API_H
#define LUA_API_H
#include "collision.h"
#include "databank.h"
#include "game_stage.h"
#include "render_util.h"
#endif

y_ptrtypedef(Body);
y_ptrtypedef(GameStage);
y_ptrtypedef(Sprite);
y_ptrtypedef(Light);
y_ptrtypedef(LuaFile);
y_ptrtypedef(Script);

/******************************************************************************/
// Vector API
/******************************************************************************/
y_api(vec)
    y_arg(y::world, x) y_arg(y::world, y)
{
  y_return(y::wvec2{x, y});
}

y_api(vec_add)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a + b);
}

y_api(vec_sub)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a - b);
}

y_api(vec_mul)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a * b);
}

y_api(vec_div)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a.euclidean_div(b));
}

y_api(vec_mod)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(y::wvec2{fmod(a[xx], b[xx]), fmod(a[yy], b[yy])});
}

y_api(vec_unm)
    y_arg(const y::wvec2, v)
{
  y_return(-v);
}

y_api(vec_eq)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a == b);
}

y_api(vec_x)
    y_arg(const y::wvec2, v)
{
  y_return(v[xx]);
}

y_api(vec_y)
    y_arg(const y::wvec2, v)
{
  y_return(v[yy]);
}

y_api(vec_normalised)
    y_arg(const y::wvec2, v)
{
  y_return(v.normalised());
}

y_api(vec_normalise)
    y_arg(y::wvec2, v)
{
  v.normalise();
  y_void();
}

y_api(vec_dot)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a.dot(b));
}

y_api(vec_length_squared)
    y_arg(const y::wvec2, v)
{
  y_return(v.length_squared());
}

y_api(vec_length)
    y_arg(const y::wvec2, v)
{
  y_return(v.length());
}

y_api(vec_angle)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a.angle(b));
}

y_api(vec_in_region)
    y_arg(const y::wvec2, v)
    y_arg(const y::wvec2, origin) y_arg(const y::wvec2, size)
{
  y_return(v.in_region(origin - size / 2, size));
}

y_api(vec_abs)
    y_arg(const y::wvec2, v)
{
  y_return(y::abs(v));
}

y_api(vec_max)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(y::max(a, b));
}

y_api(vec_min)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(y::min(a, b));
}

typedef y::wvec2 Vec;
y_valtypedef(Vec)
  y_method("__add", vec_add)
  y_method("__sub", vec_sub)
  y_method("__mul", vec_mul)
  y_method("__div", vec_div)
  y_method("__mod", vec_mod)
  y_method("__unm", vec_unm)
  y_method("__eq", vec_eq)
  y_method("x", vec_x)
  y_method("y", vec_y)
  y_method("normalised", vec_normalised)
  y_method("normalise", vec_normalise)
  y_method("dot", vec_dot)
  y_method("length_squared", vec_length_squared)
  y_method("length", vec_length)
  y_method("angle", vec_angle)
  y_method("in_region", vec_in_region)
  y_method("abs", vec_abs)
  y_method("max", vec_max)
  y_method("min", vec_min)
y_endtypedef();

/******************************************************************************/
// Script reference API
/******************************************************************************/
y_api(ref)
    y_arg(Script*, script)
{
  y_return(ScriptReference(*script));
}

y_api(ref_eq)
    y_arg(const ScriptReference, a) y_arg(const ScriptReference, b)
{
  y_return(a.get() == b.get());
}

y_api(ref_gc)
    y_arg(const ScriptReference, a)
{
  a.~ScriptReference();
  y_void();
}

y_api(ref_valid)
    y_arg(const ScriptReference, a)
{
  y_return(a.is_valid());
}

y_api(ref_get)
    y_arg(ScriptReference, a)
{
  y_return(a.get());
}

y_valtypedef(ScriptReference)
  y_method("__eq", ref_eq)
  y_method("__gc", ref_gc)
  y_method("valid", ref_valid)
  y_method("get", ref_get)
y_endtypedef();

/******************************************************************************/
// Script API
/******************************************************************************/
y_api(get_uid)
    y_arg(const Script*, script)
{
  y_return(stage.get_scripts().get_uid(script));
}

y_api(get_origin)
    y_arg(const Script*, script)
{
  y_return(script->get_origin());
}

y_api(get_region)
    y_arg(const Script*, script)
{
  y_return(script->get_region());
}

y_api(get_rotation)
    y_arg(const Script*, script)
{
  y_return(script->get_rotation());
}

y_api(set_origin)
    y_arg(Script*, script) y_arg(const y::wvec2, origin)
{
  script->set_origin(origin);
  y_void();
}

y_api(set_region)
    y_arg(Script*, script) y_arg(const y::wvec2, region)
{
  script->set_region(region);
  y_void();
}

y_api(set_rotation)
    y_arg(Script*, script) y_arg(y::world, rotation)
{
  script->set_rotation(rotation);
  y_void();
}

y_api(get_script)
    y_arg(y::string, path)
{
  y_return(&stage.get_bank().scripts.get(path));
}

y_api(destroy)
    y_arg(Script*, script)
{
  script->destroy();
  y_void();
}

y_api(create)
    y_arg(const LuaFile*, file) y_arg(const y::wvec2, origin)
{
  y_return(&stage.get_scripts().create_script(*file, origin));
}

y_api(create_region)
    y_arg(const LuaFile*, file)
    y_arg(const y::wvec2, origin) y_arg(const y::wvec2, region)
{
  y_return(&stage.get_scripts().create_script(*file, origin, region));
}

y_api(send_message)
    y_arg(Script*, script) y_arg(y::string, function_name)
    y_arg(y::vector<LuaValue>, args)
{
  stage.get_scripts().send_message(script, function_name, args);
  y_void();
}

/******************************************************************************/
// Stage API
/******************************************************************************/
y_api(get_scripts_in_region)
    y_arg(const y::wvec2, origin) y_arg(const y::wvec2, region)
{
  ScriptBank::result result;
  stage.get_scripts().get_in_region(result, origin, region);
  y_return(result);
}

y_api(get_scripts_in_radius)
    y_arg(const y::wvec2, origin) y_arg(y::world, radius)
{
  ScriptBank::result result;
  stage.get_scripts().get_in_radius(result, origin, radius);
  y_return(result);
}

/******************************************************************************/
// Player API
/******************************************************************************/
y_api(set_player)
    y_arg(Script*, script)
{
  stage.set_player(script);
  y_void();
}

y_api(clear_player)
{
  stage.set_player(y::null);
  y_void();
}

y_api(has_player)
{
  y_return(stage.get_player() != y::null);
}

y_api(get_player)
{
  y_return(stage.get_player());
}

y_api(is_key_down)
    y_arg(y::int32, key)
{
  y_return(stage.is_key_down(key));
}

y_api(set_camera)
    y_arg(const y::wvec2, camera)
{
  stage.get_camera().set_origin(camera);
  y_void();
}

y_api(get_camera)
{
  y_return(stage.get_camera().get_origin());
}

y_api(set_camera_rotation)
    y_arg(y::world, rotation)
{
  stage.get_camera().set_rotation(rotation);
  y_void();
}

y_api(get_camera_rotation)
{
  y_return(stage.get_camera().get_rotation());
}

/******************************************************************************/
// Rendering API
/******************************************************************************/
y_api(get_sprite)
    y_arg(y::string, path)
{
  y_return(&stage.get_bank().sprites.get(path));
}

y_api(render_sprite)
    y_arg(const Script*, script) y_arg(y::int32, layer)
    y_arg(const Sprite*, sprite)
    y_arg(const y::wvec2, frame_size) y_arg(const y::wvec2, frame)
    y_arg(y::world, depth) y_arg(y::world, rotation)
    y_arg(y::world, r) y_arg(y::world, g) y_arg(y::world, b)
    y_arg(y::world, a)
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
  y_void();
}

y_api(render_fog)
    y_arg(y::int32, layer) y_arg(y::world, layering_value)
    y_arg(const y::wvec2, origin) y_arg(const y::wvec2, region)
    y_arg(const y::wvec2, tex_offset) y_arg(y::world, frame)
    y_arg(y::world, r) y_arg(y::world, g) y_arg(y::world, b)
    y_arg(y::world, a)
    y_arg(y::world, fog_min) y_arg(y::world, fog_max)
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
  y_void();
}

y_api(render_reflect)
    y_arg(y::int32, layer) y_arg(y::world, layering_value)
    y_arg(const y::wvec2, origin) y_arg(const y::wvec2, region)
    y_arg(const y::wvec2, tex_offset) y_arg(y::world, frame)
    y_arg(y::world, r) y_arg(y::world, g) y_arg(y::world, b)
    y_arg(y::world, a) y_arg(y::world, reflect_mix)
    y_arg(y::world, normal_scaling)
    y_arg(y::world, normal_scaling_reflect)
    y_arg(y::world, normal_scaling_refract)
    y_arg(y::world, reflect_fade_start) y_arg(y::world, reflect_fade_end)
    y_arg(bool, flip_x) y_arg(bool, flip_y)
    y_arg(const y::wvec2, flip_axes)
    y_arg(y::world, wave_height) y_arg(y::world, wave_scale)
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
  y_void();
}

/******************************************************************************/
// Collision API
/******************************************************************************/
y_api(create_body)
    y_arg(Script*, script)
    y_arg(const y::wvec2, offset) y_arg(const y::wvec2, size)
{
  Body* body = stage.get_collision().create_obj(*script);
  body->offset = offset;
  body->size = size;
  y_return(body);
}

y_api(destroy_body)
    y_arg(const Script*, script) y_arg(Body*, body)
{
  stage.get_collision().destroy_obj(*script, body);
  y_void();
}

y_api(destroy_bodies)
    y_arg(const Script*, script)
{
  stage.get_collision().destroy_all(*script);
  y_void();
}

y_api(get_body_offset)
    y_arg(const Body*, body)
{
  y_return(body->offset);
}

y_api(get_body_size)
    y_arg(const Body*, body)
{
  y_return(body->size);
}

y_api(get_collide_mask)
    y_arg(const Body*, body)
{
  y_return(body->collide_mask);
}

y_api(set_body_offset)
    y_arg(Body*, body) y_arg(const y::wvec2, offset)
{
  body->offset = offset;
  y_void();
}

y_api(set_body_size)
    y_arg(Body*, body) y_arg(const y::wvec2, size)
{
  body->size = size;
  y_void();
}

y_api(set_collide_mask)
    y_arg(Body*, body) y_arg(y::int32, collide_mask)
{
  body->collide_mask = collide_mask;
  y_void();
}

y_api(collider_move)
    y_arg(Script*, script) y_arg(const y::wvec2, move)
{
  y_return(stage.get_collision().collider_move(*script, move));
}

y_api(collider_rotate)
   y_arg(Script*, script) y_arg(y::world, rotate)
{
  y_return(stage.get_collision().collider_rotate(*script, rotate));
}

y_api(body_check)
    y_arg(const Script*, script) y_arg(const Body*, body)
    y_arg(y::int32, collide_mask)
{
  y_return(stage.get_collision().body_check(*script, *body, collide_mask));
}

/******************************************************************************/
// Lighting API
/******************************************************************************/
y_api(create_light)
    y_arg(Script*, script)
    y_arg(y::world, full_range) y_arg(y::world, falloff_range)
{
  Light* light = stage.get_lighting().create_obj(*script);
  light->full_range = full_range;
  light->falloff_range = falloff_range;
  light->layer_value = 0.;
  y_return(light);
}

y_api(destroy_light)
    y_arg(const Script*, script) y_arg(Light*, light)
{
  stage.get_lighting().destroy_obj(*script, light);
  y_void();
}

y_api(destroy_lights)
    y_arg(const Script*, script)
{
  stage.get_lighting().destroy_all(*script);
  y_void();
}

y_api(get_light_full_range)
    y_arg(const Light*, light)
{
  y_return(light->full_range);
}

y_api(set_light_range)
    y_arg(Light*, light) y_arg(y::world, full_range)
{
  light->full_range = full_range;
  y_void();
}

y_api(get_light_falloff_range)
    y_arg(const Light*, light)
{
  y_return(light->falloff_range);
}

y_api(set_light_falloff_range)
    y_arg(Light*, light) y_arg(y::world, falloff_range)
{
  light->falloff_range = falloff_range;
  y_void();
}

y_api(get_light_layer_value)
    y_arg(const Light*, light)
{
  y_return(light->layer_value);
}

y_api(set_light_layer_value)
    y_arg(Light*, light) y_arg(y::world, layer_value)
{
  light->layer_value = layer_value;
  y_void();
}

y_api(get_light_r)
    y_arg(const Light*, light)
{
  y_return(y::world(light->colour[rr]));
}

y_api(get_light_g)
    y_arg(const Light*, light)
{
  y_return(y::world(light->colour[gg]));
}

y_api(get_light_b)
    y_arg(const Light*, light)
{
  y_return(y::world(light->colour[bb]));
}

y_api(get_light_intensity)
    y_arg(const Light*, light)
{
  y_return(y::world(light->colour[aa]));
}

y_api(set_light_colour)
    y_arg(Light*, light)
    y_arg(y::world, r) y_arg(y::world, g) y_arg(y::world, b)
{
  light->colour[rr] = r;
  light->colour[gg] = g;
  light->colour[bb] = b;
  y_void();
}

y_api(set_light_intensity)
    y_arg(Light*, light) y_arg(y::world, intensity)
{
  light->colour[aa] = intensity;
  y_void();
}
