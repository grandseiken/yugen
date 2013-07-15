-- Data structure that saves a set of script references. Internally, it is a
-- map from script unique-IDs to references.

-- Call with an existing script set (or empty table, for a new set), and
-- a list of scripts, for example from get_scripts_in_region. Returns the set
-- updated to match the list.
-- Scripts which are newly-added will have the add_function called on them;
-- those which are removed will have the remove_function called on them.
-- We assume the given new scripts are real and not invalidated themselves.
local function update_script_set(
    previous_set, new_list, add_function, remove_function)
  new_set = {}
  for _, script in ipairs(new_list) do
    uid = get_uid(script)
    if previous_set[uid] ~= nil then
      new_set[uid] = previous_set[uid]
    else
      add_function(script)
      new_set[uid] = ref(script)
    end
  end
  -- Handle old values.
  for uid, ref in pairs(previous_set) do
    if new_set[uid] == nil and ref:valid() then
      remove_function(ref:get())
    end
  end
  return new_set
end
