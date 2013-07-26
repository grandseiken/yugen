-- Code for the player.
#include "../collide.lua"
#include "../keys.lua"
#include "../render.lua"

-- Collider body and check bodies.
local body = self:create_body(vec(0, 8), vec(6, 16))
body:set_collide_type(COLLIDE_PLAYER)
local collide_mask = COLLIDE_WORLD + COLLIDE_OBJECT
body:set_collide_mask(collide_mask)
local up_check, down_check, left_check, right_check =
    create_all_checks(self, vec(0, 8), vec(6, 16))

-- Test light.
local light = self:create_light(128, 128)
light:set_colour(1, 1, 1)

-- Jump constants.
local JUMP_STAGE_NONE = 0
local JUMP_STAGE_UP = 1
local JUMP_STAGE_CHANGE_UP = 2
local JUMP_STAGE_CAP = 3
local JUMP_STAGE_CHANGE_DOWN = 4
local JUMP_STAGE_TOTAL = 5

local JUMP_PERIOD_UP_MINIMUM = 10
local JUMP_PERIOD_UP = 28
local JUMP_PERIOD_CHANGE = 10
local JUMP_PERIOD_CAP = 5

local JUMP_ALLOWANCE = 4
local JUMP_ALLOWANCE_WALL = 6

-- Jump variables.
local jump_input_buffer = 0
local jump_timer = 0
local jump_stage = JUMP_STAGE_NONE
local wall_jump_timer = 0
local wall_jump_left = false

-- Events.
function on_submerge(amount, water, v)
  light:set_layer_value(.2 + .05 * amount)
end

function on_emerge()
  light:set_layer_value(0)
end

-- Jump behaviour.
local function jump_input()
  jump_input_buffer = JUMP_ALLOWANCE
end

local function jump_logic(left_down, right_down, up_down,
                          left_check_now, right_check_now, up_check_now,
                          down_check_start, down_check_now)
  -- Control wall-jumping.
  if left_check_now then
    wall_jump_timer = JUMP_ALLOWANCE_WALL
    wall_jump_left = true
  end
  if right_check_now then
    wall_jump_timer = JUMP_ALLOWANCE_WALL
    wall_jump_left = false
  end
  if down_check_now or up_check_now then
    wall_jump_timer = 0
  end
  if wall_jump_timer > 0 then
    if jump_input_buffer == JUMP_ALLOWANCE and
        not (down_check_now or down_check_start) and
        (wall_jump_left and right_down and not left_down or
         not wall_jump_left and left_down and not right_down) then
      jump_stage = JUMP_STAGE_UP
      jump_timer = JUMP_PERIOD_UP
      jump_input_buffer = 0
    end
    wall_jump_timer = wall_jump_timer - 1
  end

  -- Gives a small allowance for pressing jump key before landing.
  if jump_input_buffer > 0 then
    jump_input_buffer = jump_input_buffer - 1
    if jump_stage == JUMP_STAGE_NONE and
        (down_check_now or down_check_start) then
      jump_stage = JUMP_STAGE_UP
      jump_timer = JUMP_PERIOD_UP
      jump_input_buffer = 0
    end
  end

  -- Accelerate off cliffs.
  if jump_stage == JUMP_STAGE_NONE and
      not down_check_now and down_check_start then
    jump_stage = JUMP_STAGE_CHANGE_DOWN
    jump_timer = JUMP_PERIOD_CHANGE
  end

  -- Stop going upwards when we bash a ceiling.
  if up_check_now and (
      jump_stage == JUMP_STAGE_UP or jump_stage == JUMP_STAGE_CHANGE_UP) then
    if jump_stage == JUMP_STAGE_UP then
      jump_timer = jump_timer + JUMP_PERIOD_CHANGE
    end
    jump_timer = jump_timer + JUMP_PERIOD_CAP
    jump_stage = JUMP_STAGE_CAP
  end

  -- Controls minimum jump height.
  if not up_down and jump_stage == JUMP_STAGE_UP and
      jump_timer < JUMP_PERIOD_UP - JUMP_PERIOD_UP_MINIMUM then
    jump_stage = JUMP_STAGE_CHANGE_UP
    jump_timer = JUMP_PERIOD_CHANGE
  end
end

-- Controls jump sequence.
local function jump_timer_logic(down_check_now)
  if jump_timer > 0 then
    jump_timer = jump_timer - 1
  end
  if jump_timer == 0 and jump_stage ~= JUMP_STAGE_NONE then
    jump_stage = (jump_stage + 1) % JUMP_STAGE_TOTAL
    jump_timer =
        jump_stage == JUMP_STAGE_CHANGE_UP and JUMP_PERIOD_CHANGE or
        jump_stage == JUMP_STAGE_CAP and JUMP_PERIOD_CAP or
        jump_stage == JUMP_STAGE_CHANGE_DOWN and JUMP_PERIOD_CHANGE or 0
  end
  -- Cancel jump when we hit the ground.
  if down_check_now and jump_stage >= JUMP_STAGE_CAP then
    jump_timer = 0
    jump_stage = JUMP_STAGE_NONE
  end
end

-- Controls jump arc.
local function y_multiplier()
  if jump_stage == JUMP_STAGE_UP then
    return -1
  elseif jump_stage == JUMP_STAGE_CHANGE_UP then
    return -jump_timer / JUMP_PERIOD_CHANGE
  elseif jump_stage == JUMP_STAGE_CAP then
    return 0
  elseif jump_stage == JUMP_STAGE_CHANGE_DOWN then
    return (JUMP_PERIOD_CHANGE - jump_timer) / JUMP_PERIOD_CHANGE
  end
  return 1
end

local function step_multiplier()
  if jump_stage == JUMP_STAGE_NONE then
    return -1
  elseif jump_stage < JUMP_STAGE_CAP then
    return 0
  else
    return 1
  end
end

local function is_jumping()
  return jump_stage ~= JUMP_STAGE_NONE
end

-- Movement constants.
local GRAVITY = 4
local MOVE_SPEED = 2.5

function update()
  local left_down = is_key_down(KEY_LEFT)
  local right_down = is_key_down(KEY_RIGHT)
  local up_down = is_key_down(KEY_UP)

  local v = 0
  if left_down then
    v = v - MOVE_SPEED
  end
  if right_down then
    v = v + MOVE_SPEED
  end

  -- Need to know the down check before the x-axis move for applying
  -- acceleration when we go off a cliff, and being able to jump when
  -- running down a slope.
  local down_check_start = down_check:body_check(collide_mask)
  local down_check_now = false

  -- Handle x-axis movement with stepping up slopes (or down overhangs when
  -- in the air).
  local original_y = self:get_origin():y()
  local step_amount = MOVE_SPEED * step_multiplier()
  if step_amount ~= 0 then
    self:collider_move(vec(0, step_amount))
  end
  self:collider_move(vec(v, 0))
  if step_amount ~= 0 then
    self:collider_move(vec(0, original_y - self:get_origin():y()))
  end

  -- Step down if we're not jumping to stick to down ramps; if this doesn't
  -- end up with us touching the ground then reverse it (since it's not a
  -- slope or is too steep).
  if not is_jumping() then
    original_y = self:get_origin():y()
    self:collider_move(vec(0, MOVE_SPEED))
    down_check_now = down_check:body_check(collide_mask)
    if not down_check_now then
      self:collider_move(vec(0, original_y - self:get_origin():y()))
      down_check_now = down_check:body_check(collide_mask)
    end
  else
    down_check_now = down_check:body_check(collide_mask)
  end

  local up_check_now = up_check:body_check(collide_mask)
  local left_check_now = left_check:body_check(collide_mask)
  local right_check_now = right_check:body_check(collide_mask)

  -- Handle y-axis movement.
  jump_logic(left_down, right_down, up_down,
             left_check_now, right_check_now, up_check_now,
             down_check_start, down_check_now)
  self:collider_move(vec(0, GRAVITY * y_multiplier()))
  jump_timer_logic(down_check_now)
  set_camera_rotation(get_camera_rotation() + math.pi / 1024)
end

function key(k)
  if k == KEY_UP then
    jump_input()
  end
end

-- Sprite constants.
local sprite = get_sprite("/tiles/easy.png")
local frame_size = vec(32, 32)
local frame = vec(3, 16)

function draw()
  render_sprite_self(sprite, frame_size, frame, 0.3)
end
