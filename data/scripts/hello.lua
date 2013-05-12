-- hello.lua --
local sprite = get_sprite("/tiles/temple.png")
local script = get_script("/scripts/hello2.lua")
local counter = 0
function update()
  set_origin(self, vec(1, 1) + get_origin(self))
  counter = counter + 1
  if counter >= 64 then
    counter = counter - 64
    create(script, get_origin(self))
  end
end
function draw()
  render_sprite(self, sprite, vec(32, 32), vec(0, 0), 0.0)
end
