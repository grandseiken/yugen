// Magic include file doesn't need preprocessor guards. Types and functions
// defined here with the ylib macros are automatically exposed to Lua.
#ifndef LUA_API_H
#define LUA_API_H
#include "collision.h"
#include "databank.h"
#include "game_stage.h"
#include "render_util.h"
#endif

ylib_typedef(Body);
ylib_typedef(GameStage);
ylib_typedef(Sprite);
ylib_typedef(Light);
ylib_typedef(LuaFile);
ylib_typedef(Script);

/******************************************************************************/
// Vector API
/******************************************************************************/
ylib_api(vec)
    ylib_arg(y::world, x) ylib_arg(y::world, y)
{
  ylib_return(y::wvec2{x, y});
}

ylib_api(vec_add)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a + b);
}

ylib_api(vec_sub)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a - b);
}

ylib_api(vec_mul)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a * b);
}

ylib_api(vec_div)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a.euclidean_div(b));
}

ylib_api(vec_mod)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(y::wvec2{fmod(a[xx], b[xx]), fmod(a[yy], b[yy])});
}

ylib_api(vec_unm)
    ylib_refarg(const y::wvec2, v)
{
  ylib_return(-v);
}

ylib_api(vec_eq)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a == b);
}

ylib_api(vec_x)
    ylib_refarg(const y::wvec2, v)
{
  ylib_return(v[xx]);
}

ylib_api(vec_y)
    ylib_refarg(const y::wvec2, v)
{
  ylib_return(v[yy]);
}

ylib_api(vec_normalised)
    ylib_refarg(const y::wvec2, v)
{
  ylib_return(v.normalised());
}

ylib_api(vec_normalise)
    ylib_refarg(y::wvec2, v)
{
  v.normalise();
  ylib_void();
}

ylib_api(vec_dot)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(a.dot(b));
}

ylib_api(vec_length_squared)
    ylib_refarg(const y::wvec2, v)
{
  ylib_return(v.length_squared());
}

ylib_api(vec_length)
    ylib_refarg(const y::wvec2, v)
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

ylib_api(vec_abs)
    ylib_refarg(const y::wvec2, v)
{
  ylib_return(y::abs(v));
}

ylib_api(vec_max)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(y::max(a, b));
}

ylib_api(vec_min)
    ylib_refarg(const y::wvec2, a) ylib_refarg(const y::wvec2, b)
{
  ylib_return(y::min(a, b));
}

/******************************************************************************/
// Script reference API
/******************************************************************************/
ylib_api(ref)
    ylib_arg(Script*, script)
{
  ylib_return(ScriptReference(*script));
}

ylib_api(ref_eq)
    ylib_refarg(const ScriptReference, a) ylib_refarg(const ScriptReference, b)
{
  ylib_return(a.get() == b.get());
}

ylib_api(ref_gc)
    ylib_refarg(const ScriptReference, a)
{
  a.~ScriptReference();
  ylib_void();
}

ylib_api(ref_valid)
    ylib_refarg(const ScriptReference, a)
{
  ylib_return(a.is_valid());
}

ylib_api(ref_get)
    ylib_refarg(ScriptReference, a)
{
  ylib_return(a.get());
}

/******************************************************************************/
// Script API
/******************************************************************************/
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

ylib_api(set_rotation)
    ylib_arg(Script*, script) ylib_arg(y::world, rotation)
{
  script->set_rotation(rotation);
  ylib_void();
}

ylib_api(get_sprite)
    ylib_arg(y::string, path)
{
  ylib_return(&stage.get_bank().sprites.get(path));
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
    ylib_arg(const LuaFile*, file) ylib_refarg(const y::wvec2, origin)
{
  ylib_return(&stage.get_scripts().create_script(*file, origin));
}

ylib_api(create_region)
    ylib_arg(const LuaFile*, file)
    ylib_refarg(const y::wvec2, origin) ylib_refarg(const y::wvec2, region)
{
  ylib_return(&stage.get_scripts().create_script(*file, origin, region));
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
    ylib_refarg(const y::wvec2, camera)
{
  stage.set_camera(camera);
  ylib_void();
}

ylib_api(get_camera)
{
  ylib_return(stage.get_camera());
}

ylib_api(set_camera_rotation)
    ylib_arg(y::world, rotation)
{
  stage.set_camera_rotation(rotation);
  ylib_void();
}

ylib_api(get_camera_rotation)
{
  ylib_return(stage.get_camera_rotation());
}

/******************************************************************************/
// Rendering API
/******************************************************************************/
ylib_api(render_sprite)
    ylib_arg(const Script*, script) ylib_arg(y::int32, layer)
    ylib_arg(const Sprite*, sprite)
    ylib_refarg(const y::wvec2, frame_size) ylib_refarg(const y::wvec2, frame)
    ylib_arg(y::world, depth) ylib_arg(y::world, rotation)
    ylib_arg(y::world, r) ylib_arg(y::world, g) ylib_arg(y::world, b)
    ylib_arg(y::world, a)
{
  GameStage::draw_stage ds = stage.get_current_draw_stage();

  RenderBatch& batch = stage.get_current_batch();
  const GlTexture2D& texture = stage.draw_stage_is_normal(ds) ?
      sprite->normal : sprite->texture;
  y::wvec2 origin = script->get_origin() - frame_size / 2;

  if (stage.draw_stage_is_layer(ds, GameStage::draw_layer(layer))) {
    stage.set_current_draw_any();
    batch.add_sprite(texture, y::ivec2(frame_size),
                     y::fvec2(origin), y::ivec2(frame),
                     depth, rotation,
                     y::fvec4{float(r), float(g), float(b), float(a)});
  }
  ylib_void();
}

ylib_api(render_fog)
    ylib_arg(y::int32, layer)
    ylib_refarg(const y::wvec2, origin) ylib_refarg(const y::wvec2, region)
    ylib_refarg(const y::wvec2, tex_offset) ylib_arg(y::world, frame)
    ylib_arg(y::world, r) ylib_arg(y::world, g) ylib_arg(y::world, b)
    ylib_arg(y::world, a)
    ylib_arg(y::world, fog_min) ylib_arg(y::world, fog_max)
{
  GameStage::draw_stage ds = stage.get_current_draw_stage();

  if (stage.draw_stage_is_layer(ds, GameStage::draw_layer(layer))) {
    stage.set_current_draw_any();
    if (stage.draw_stage_is_normal(ds)) {
      stage.get_environment().render_fog_normal(
          stage.get_util(), origin, region);
    }
    else {
      stage.get_environment().render_fog_colour(
          stage.get_util(), origin, region, tex_offset, fog_min, fog_max, frame,
          y::fvec4{float(r), float(g), float(b), float(a)});
    }
  }
  ylib_void();
}

ylib_api(render_reflect)
    ylib_arg(y::int32, layer)
    ylib_refarg(const y::wvec2, origin) ylib_refarg(const y::wvec2, region)
    ylib_refarg(const y::wvec2, tex_offset) ylib_arg(y::world, frame)
    ylib_arg(y::world, r) ylib_arg(y::world, g) ylib_arg(y::world, b)
    ylib_arg(y::world, a) ylib_arg(y::world, reflect_mix)
    ylib_arg(y::world, normal_scaling)
    ylib_arg(y::world, normal_scaling_reflect)
    ylib_arg(y::world, normal_scaling_refract)
    ylib_arg(y::world, reflect_fade_start) ylib_arg(y::world, reflect_fade_end)
    ylib_arg(y::world, wave_height) ylib_arg(y::world, wave_scale)
    ylib_arg(bool, flip_x) ylib_arg(bool, flip_y)
    ylib_refarg(const y::wvec2, flip_axes)
{
  GameStage::draw_stage ds = stage.get_current_draw_stage();

  if (stage.draw_stage_is_layer(ds, GameStage::draw_layer(layer))) {
    stage.set_current_draw_any();
    if (stage.draw_stage_is_normal(ds)) {
      stage.get_environment().render_reflect_normal(
          stage.get_util(), origin, region, tex_offset, frame, normal_scaling);
    }
    else {
      stage.get_environment().render_reflect_colour(
          stage.get_util(), origin, region, tex_offset, frame,
          y::fvec4{float(r), float(g), float(b), float(a)},
          reflect_mix, normal_scaling_reflect, normal_scaling_refract,
          reflect_fade_start, reflect_fade_end, wave_height, wave_scale,
          flip_x, flip_y, flip_axes, stage.get_framebuffer());
    }
  }
  ylib_void();
}

/******************************************************************************/
// Collision API
/******************************************************************************/
ylib_api(create_body)
    ylib_arg(Script*, script)
    ylib_refarg(const y::wvec2, offset) ylib_refarg(const y::wvec2, size)
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
    ylib_arg(Body*, body) ylib_refarg(const y::wvec2, offset)
{
  body->offset = offset;
  ylib_void();
}

ylib_api(set_body_size)
    ylib_arg(Body*, body) ylib_refarg(const y::wvec2, size)
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
    ylib_arg(Script*, script) ylib_refarg(const y::wvec2, move)
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

ylib_api(get_light_r)
    ylib_arg(const Light*, light)
{
  ylib_return(light->colour[rr]);
}

ylib_api(get_light_g)
    ylib_arg(const Light*, light)
{
  ylib_return(light->colour[gg]);
}

ylib_api(get_light_b)
    ylib_arg(const Light*, light)
{
  ylib_return(light->colour[bb]);
}

ylib_api(get_light_intensity)
    ylib_arg(const Light*, light)
{
  ylib_return(light->colour[aa]);
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
