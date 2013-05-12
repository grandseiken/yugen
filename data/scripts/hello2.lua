-- hello2.lua --
local sprite = get_sprite("/tiles/temple.png")
local counter = 0
function update()
  set_origin(self, vec(1, -1) + get_origin(self))
  counter = counter + 1
  if counter == 80 then
    destroy(self)
  end
end
function draw()
  render_sprite(self, sprite, vec(32, 32), vec(1, 0), 0.0)
end
