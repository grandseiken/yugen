-- hello.lua --
function update()
  origin = get_origin(self)
  set_origin(self, {1 + origin[1], 1 + origin[2]})
end
function draw()
  render_sprite(self, "/yedit/missing.png", 32, 32, 0, 0, 0.0)
end
