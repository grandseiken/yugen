-- hello.lua --
local sprite = get_sprite("/tiles/temple.png")
local script = get_script("/scripts/hello2.lua")
local counter = 0
local body = create_body(self, vec(0, 8), vec(32, 16))
function update()
  collider_move(self, vec(1, 1))
  counter = counter + 1
  if counter >= 64 then
    counter = counter - 64
    create(script, get_origin(self))
  end
end
function draw()
  render_sprite(self, sprite, vec(32, 32), vec(0, 0), 0.0)
end
