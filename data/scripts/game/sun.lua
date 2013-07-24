-- Code for sunlight.
local light = self:create_light(2048, 2048)
light:set_colour(1, 1, 1)
light:set_offset(vec(self:get_region():x() * 10000, 0))
light:set_layer_value(0)
local angle = 0

function yedit_colour()
  return .8, .8, 0
end

function update()
  angle = angle + math.pi / 512
  local v = vec(math.sin(angle), math.cos(angle))
  if v:y() < 0 then
    v = vec(-v:x(), -v:y())
  end
  light:set_normal_vec(v)
end
