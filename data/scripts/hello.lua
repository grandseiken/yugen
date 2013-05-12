-- hello.lua --
local sprite = get_sprite("/tiles/temple.png")
function update()
  set_origin(self, vec(1, 1) + get_origin(self))
end
function draw()
  render_sprite(self, sprite, vec(32, 32), vec(0, 0), 0.0)
end
