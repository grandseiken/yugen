// Magic include file doesn't need preprocessor guards. Types and functions
// defined here with the y_* macros are automatically exposed to Lua.
#ifndef LUA_API_H
#define LUA_API_H
#include "collision.h"
#include "databank.h"
#include "game_stage.h"
#include "render_util.h"
#endif

y_ptrtypedef(GameStage) {} y_endtypedef();
y_ptrtypedef(Sprite) {} y_endtypedef();
y_ptrtypedef(LuaFile) {} y_endtypedef();

/******************************************************************************/
// Vector API
/******************************************************************************/
y_api(vec)
    y_arg(y::world, x) y_arg(y::world, y)
{
  y_return(y::wvec2{x, y});
}

y_api(vec__add)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a + b);
}

y_api(vec__sub)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a - b);
}

y_api(vec__mul)
    y_optarg(const y::wvec2, a_vec) y_optarg(y::world, scalar)
    y_optarg(const y::wvec2, b_vec)
{
  static const y::string error =
           LuaType<y::wvec2>::type_name + " or " +
           LuaType<y::world>::type_name + " expected";
  y_assert(a_vec_defined || b_vec_defined, 0, error);
  y_assert(scalar_defined || (a_vec_defined && b_vec_defined), 1, error);

  y_return((a_vec_defined ? a_vec : y::wvec2(scalar, scalar)) *
           (b_vec_defined ? b_vec : y::wvec2(scalar, scalar)));
}

y_api(vec__div)
    y_optarg(const y::wvec2, a_vec) y_optarg(y::world, scalar)
    y_optarg(const y::wvec2, b_vec)
{
  static const y::string error =
           LuaType<y::wvec2>::type_name + " or " +
           LuaType<y::world>::type_name + " expected";
  y_assert(a_vec_defined || b_vec_defined, 0, error);
  y_assert(scalar_defined || (a_vec_defined && b_vec_defined), 1, error);

  y_return((a_vec_defined ? a_vec : y::wvec2(scalar, scalar)) /
           (b_vec_defined ? b_vec : y::wvec2(scalar, scalar)));
}

y_api(vec__mod)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(y::wvec2{fmod(a[xx], b[xx]), fmod(a[yy], b[yy])});
}

y_api(vec__unm)
    y_arg(const y::wvec2, v)
{
  y_return(-v);
}

y_api(vec__x)
    y_arg(const y::wvec2, v)
{
  y_return(v[xx]);
}

y_api(vec__y)
    y_arg(const y::wvec2, v)
{
  y_return(v[yy]);
}

y_api(vec__normalised)
    y_arg(const y::wvec2, v)
{
  y_return(v.normalised());
}

y_api(vec__normalise)
    y_arg(y::wvec2, v)
{
  v.normalise();
  y_void();
}

y_api(vec__dot)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a.dot(b));
}

y_api(vec__length_squared)
    y_arg(const y::wvec2, v)
{
  y_return(v.length_squared());
}

y_api(vec__length)
    y_arg(const y::wvec2, v)
{
  y_return(v.length());
}

y_api(vec__angle)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(a.angle(b));
}

y_api(vec__in_region)
    y_arg(const y::wvec2, v)
    y_arg(const y::wvec2, origin) y_arg(const y::wvec2, size)
{
  y_return(v.in_region(origin - size / 2, size));
}

y_api(vec__abs)
    y_arg(const y::wvec2, v)
{
  y_return(y::abs(v));
}

y_api(vec__max)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(y::max(a, b));
}

y_api(vec__min)
    y_arg(const y::wvec2, a) y_arg(const y::wvec2, b)
{
  y_return(y::min(a, b));
}

typedef y::wvec2 Vec;
y_valtypedef(Vec) {
  y_method("__add", vec__add);
  y_method("__sub", vec__sub);
  y_method("__mul", vec__mul);
  y_method("__div", vec__div);
  y_method("__mod", vec__mod);
  y_method("__unm", vec__unm);
  y_method("x", vec__x);
  y_method("y", vec__y);
  y_method("normalised", vec__normalised);
  y_method("normalise", vec__normalise);
  y_method("dot", vec__dot);
  y_method("length_squared", vec__length_squared);
  y_method("length", vec__length);
  y_method("angle", vec__angle);
  y_method("in_region", vec__in_region);
  y_method("abs", vec__abs);
  y_method("max", vec__max);
  y_method("min", vec__min);
} y_endtypedef();

/******************************************************************************/
// Script reference API
/******************************************************************************/
y_api(ref)
    y_arg(Script*, script)
{
  y_return(ScriptReference(*script));
}

y_api(ref__gc)
    y_arg(const ScriptReference, a)
{
  a.~ScriptReference();
  y_void();
}

y_api(ref__valid)
    y_arg(const ScriptReference, a)
{
  y_return(a.is_valid());
}

y_api(ref__get)
    y_arg(ScriptReference, a)
{
  y_return(a.get());
}

y_valtypedef(ScriptReference) {
  y_method("__gc", ref__gc);
  y_method("valid", ref__valid);
  y_method("get", ref__get);
} y_endtypedef();

/******************************************************************************/
// Script API
/******************************************************************************/
y_api(script__get_uid)
    y_arg(const Script*, script)
{
  y_return(stage.get_scripts().get_uid(script));
}

y_api(script__get_region)
    y_arg(const Script*, script)
{
  y_return(script->get_region());
}

y_api(script__get_origin)
    y_arg(const Script*, script)
{
  y_return(script->get_origin());
}

y_api(script__get_rotation)
    y_arg(const Script*, script)
{
  y_return(script->get_rotation());
}

y_api(script__set_region)
    y_arg(Script*, script) y_arg(const y::wvec2, region)
{
  script->set_region(region);
  y_void();
}

y_api(script__set_origin)
    y_arg(Script*, script) y_arg(const y::wvec2, origin)
{
  script->set_origin(origin);
  y_void();
}

y_api(script__set_rotation)
    y_arg(Script*, script) y_arg(y::world, rotation)
{
  script->set_rotation(rotation);
  y_void();
}

y_api(script__destroy)
    y_arg(Script*, script)
{
  script->destroy();
  y_void();
}

y_api(script__send_message)
    y_arg(Script*, script) y_arg(y::string, function_name)
    y_varargs(LuaValue, args)
{
  stage.get_scripts().send_message(script, function_name, args);
  y_void();
}

y_api(script__create_body)
    y_arg(Script*, script)
    y_arg(const y::wvec2, offset) y_arg(const y::wvec2, size)
{
  Body* body = stage.get_collision().create_obj(*script);
  body->offset = offset;
  body->size = size;
  y_return(body);
}

y_api(script__destroy_bodies)
    y_arg(const Script*, script)
{
  stage.get_collision().destroy_all(*script);
  y_void();
}

y_api(script__destroy_body)
    y_arg(const Script*, script) y_arg(Body*, body)
{
  stage.get_collision().destroy_obj(*script, body);
  y_void();
}

y_api(script__collider_move)
    y_arg(Script*, script) y_arg(const y::wvec2, move)
{
  y_return(stage.get_collision().collider_move(*script, move));
}

y_api(script__collider_rotate)
   y_arg(Script*, script) y_arg(y::world, rotate)
   y_optarg(const y::wvec2, origin_offset)
{
  y_return(stage.get_collision().collider_rotate(
      *script, rotate, origin_offset_defined ? origin_offset : y::wvec2()));
}

y_api(script__body_check)
    y_arg(const Script*, script) y_arg(const Body*, body)
    y_arg(y::int32, collide_mask)
{
  y_return(stage.get_collision().body_check(*script, *body, collide_mask));
}

y_api(script__create_light)
    y_arg(Script*, script)
    y_arg(y::world, full_range) y_arg(y::world, falloff_range)
{
  Light* light = stage.get_lighting().create_obj(*script);
  light->full_range = full_range;
  light->falloff_range = falloff_range;
  light->layer_value = 0.;
  y_return(light);
}

y_api(script__destroy_lights)
    y_arg(const Script*, script)
{
  stage.get_lighting().destroy_all(*script);
  y_void();
}

y_api(script__destroy_light)
    y_arg(const Script*, script) y_arg(Light*, light)
{
  stage.get_lighting().destroy_obj(*script, light);
  y_void();
}

y_ptrtypedef(Script) {
  y_method("get_uid", script__get_uid);
  y_method("get_region", script__get_region);
  y_method("get_origin", script__get_origin);
  y_method("get_rotation", script__get_rotation);
  y_method("set_region", script__set_region);
  y_method("set_origin", script__set_origin);
  y_method("set_rotation", script__set_rotation);
  y_method("destroy", script__destroy);
  y_method("send_message", script__send_message);
  y_method("create_body", script__create_body);
  y_method("destroy_bodies", script__destroy_bodies);
  y_method("destroy_body", script__destroy_body);
  y_method("collider_move", script__collider_move);
  y_method("collider_rotate", script__collider_rotate);
  y_method("body_check", script__body_check);
  y_method("create_light", script__create_light);
  y_method("destroy_lights", script__destroy_lights);
  y_method("destroy_light", script__destroy_light);
} y_endtypedef();

/******************************************************************************/
// Game stage API
/******************************************************************************/
y_api(get_script)
    y_arg(y::string, path)
{
  y_return(&stage.get_bank().scripts.get(path));
}

y_api(create_script)
    y_arg(const LuaFile*, file) y_arg(const y::wvec2, origin)
{
  y_return(&stage.get_scripts().create_script(*file, origin));
}

y_api(create_script_region)
    y_arg(const LuaFile*, file)
    y_arg(const y::wvec2, origin) y_arg(const y::wvec2, region)
{
  y_return(&stage.get_scripts().create_script(*file, origin, region));
}

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
    y_arg(y::int32, layer) y_arg(y::world, depth)
    y_arg(const y::wvec2, origin) y_arg(y::world, rotation)
    y_arg(const Sprite*, sprite)
    y_arg(const y::wvec2, frame_size) y_arg(const y::wvec2, frame)
    y_arg(y::world, r) y_arg(y::world, g) y_arg(y::world, b)
    y_arg(y::world, a)
{
  const GameRenderer& renderer = stage.get_renderer();
  bool normal = renderer.draw_pass_is_normal();

  RenderBatch& batch = renderer.get_current_batch();
  const GlTexture2D& texture = normal ? sprite->normal : sprite->texture;

  if (renderer.draw_pass_is_layer(GameRenderer::draw_layer(layer))) {
    renderer.set_current_draw_any();
    batch.add_sprite(texture, y::ivec2(frame_size), normal,
                     y::fvec2(origin - frame_size / 2), y::ivec2(frame),
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

  if (renderer.draw_pass_is_layer(GameRenderer::draw_layer(layer))) {
    renderer.set_current_draw_any();
    if (renderer.draw_pass_is_normal()) {
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

  if (renderer.draw_pass_is_layer(GameRenderer::draw_layer(layer))) {
    renderer.set_current_draw_any();
    if (renderer.draw_pass_is_normal()) {
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
y_api(body__get_offset)
    y_arg(const Body*, body)
{
  y_return(body->offset);
}

y_api(body__get_size)
    y_arg(const Body*, body)
{
  y_return(body->size);
}

y_api(body__get_collide_mask)
    y_arg(const Body*, body)
{
  y_return(body->collide_mask);
}

y_api(body__set_offset)
    y_arg(Body*, body) y_arg(const y::wvec2, offset)
{
  body->offset = offset;
  y_void();
}

y_api(body__set_size)
    y_arg(Body*, body) y_arg(const y::wvec2, size)
{
  body->size = size;
  y_void();
}

y_api(body__set_collide_mask)
    y_arg(Body*, body) y_arg(y::int32, collide_mask)
{
  body->collide_mask = collide_mask;
  y_void();
}

y_api(body__get_source)
    y_arg(Body*, body)
{
  y_return(&body->source);
}

y_api(body__body_check)
    y_arg(Body*, body) y_arg(y::int32, collide_mask)
{
  y_return(stage.get_collision().body_check(body->source,
                                            *body, collide_mask));
}

y_api(body__destroy)
    y_arg(Body*, body)
{
  stage.get_collision().destroy_obj(body->source, body);
  y_void();
}

y_ptrtypedef(Body) {
  y_method("get_offset", body__get_offset);
  y_method("get_size", body__get_size);
  y_method("get_collide_mask", body__get_collide_mask);
  y_method("set_offset", body__set_offset);
  y_method("set_size", body__set_size);
  y_method("set_collide_mask", body__set_collide_mask);
  y_method("get_source", body__get_source);
  y_method("body_check", body__body_check);
  y_method("destroy", body__destroy);
} y_endtypedef();

/******************************************************************************/
// Lighting API
/******************************************************************************/
y_api(light__get_offset)
    y_arg(const Light*, light)
{
  y_return(light->offset);
}

y_api(light__set_offset)
    y_arg(Light*, light) y_arg(const y::wvec2, offset)
{
  light->offset = offset;
  y_void();
}

y_api(light__get_normal_vec)
    y_arg(const Light*, light)
{
  y_return(light->normal_vec);
}

y_api(light__set_normal_vec)
    y_arg(Light*, light) y_arg(const y::wvec2, normal_vec)
{
  light->normal_vec = normal_vec.normalised();
  y_void();
}

y_api(light__get_full_range)
    y_arg(const Light*, light)
{
  y_return(light->full_range);
}

y_api(light__set_full_range)
    y_arg(Light*, light) y_arg(y::world, full_range)
{
  light->full_range = full_range;
  y_void();
}

y_api(light__get_falloff_range)
    y_arg(const Light*, light)
{
  y_return(light->falloff_range);
}

y_api(light__set_falloff_range)
    y_arg(Light*, light) y_arg(y::world, falloff_range)
{
  light->falloff_range = falloff_range;
  y_void();
}

y_api(light__get_layer_value)
    y_arg(const Light*, light)
{
  y_return(light->layer_value);
}

y_api(light__set_layer_value)
    y_arg(Light*, light) y_arg(y::world, layer_value)
{
  light->layer_value = layer_value;
  y_void();
}

y_api(light__get_colour)
    y_arg(const Light*, light)
{
  y_return(y::world(light->colour[rr]),
           y::world(light->colour[gg]),
           y::world(light->colour[bb]));
}

y_api(light__set_colour)
    y_arg(Light*, light)
    y_arg(y::world, r) y_arg(y::world, g) y_arg(y::world, b)
{
  light->colour[rr] = r;
  light->colour[gg] = g;
  light->colour[bb] = b;
  y_void();
}

y_api(light__get_intensity)
    y_arg(const Light*, light)
{
  y_return(y::world(light->colour[aa]));
}

y_api(light__set_intensity)
    y_arg(Light*, light) y_arg(y::world, intensity)
{
  light->colour[aa] = intensity;
  y_void();
}

y_api(light__get_angle)
    y_arg(const Light*, light)
{
  y_return(light->angle);
}

y_api(light__set_angle)
    y_arg(Light*, light) y_arg(y::world, angle)
{
  light->angle = y::angle(angle);
  y_void();
}

y_api(light__get_aperture)
    y_arg(const Light*, light)
{
  y_return(light->aperture);
}

y_api(light__set_aperture)
    y_arg(Light*, light) y_arg(y::world, aperture)
{
  light->aperture = aperture;
  y_void();
}

y_api(light__get_source)
    y_arg(Light*, light)
{
  y_return(&light->source);
}

y_api(light__destroy)
    y_arg(Light*, light)
{
  stage.get_lighting().destroy_obj(light->source, light);
  y_void();
}

y_ptrtypedef(Light) {
  y_method("get_offset", light__get_offset);
  y_method("set_offset", light__set_offset);
  y_method("get_normal_vec", light__get_normal_vec);
  y_method("set_normal_vec", light__set_normal_vec);
  y_method("get_full_range", light__get_full_range);
  y_method("set_full_range", light__set_full_range);
  y_method("get_falloff_range", light__get_falloff_range);
  y_method("set_falloff_range", light__set_falloff_range);
  y_method("get_layer_value", light__get_layer_value);
  y_method("set_layer_value", light__set_layer_value);
  y_method("get_colour", light__get_colour);
  y_method("set_colour", light__set_colour);
  y_method("get_intensity", light__get_intensity);
  y_method("set_intensity", light__set_intensity);
  y_method("get_angle", light__get_angle);
  y_method("set_angle", light__set_angle);
  y_method("get_aperture", light__get_aperture);
  y_method("set_aperture", light__set_aperture);
  y_method("get_source", light__get_source);
  y_method("destroy", light__destroy);
} y_endtypedef();
