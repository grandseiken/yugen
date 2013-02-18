#include "filesystem.h"

bool check_pattern(const std::string& path, const std::string& pattern)
{
  // TODO: implement
  return false;
}

void Filesystem::list_pattern(y::string_vector& output,
                              const y::string& pattern) const
{
  y::size star_pos = pattern.find_first_of('*');
  if (star_pos == std::npos) {
    if (exists(path)) {
      output.push_back(pattern);
    }
    return;
  }

  y::size left = pattern.find_last_of('/', star_pos);
  y::size right = pattern.find_first_of('/', star_pos);

  y::string prefix = pattern.substr(0, left);
  y::string_vector intermediate;

  if (pattern.find_first_of("**", star_pos) < right) {
    list_directory_recursive(intermedate, prefix);
    for (const y::string& path : intermediate) {
      if check_pattern(path, pattern) {
        output.push_back(path);
      }
    }
    return;
  } 

  pattern_rel = pattern.substr(left + 1, right - left - 1);
  list_directory(intermediate, prefix);

  for (const y::string& path : intermediate) {
    y::string path_rel = path.substr(left + 1);
    if (!check_pattern(path_rel, pattern_rel)) {
      continue;
    }

    if (right == std::npos) {
      output.push_back(path);
    }
    else if (is_directory(path)) {
      list_pattern(output,
                   prefix + '/' + path_rel + '/' + pattern.substr(right + 1)); 
    }
  }
}

void Filesystem::list_directory_recursive(const y::string_vector& output,
                                          const y::string& path) const
{
  y::string_vector directory;
  list_directory(directory, path);

  for (const y::string& sub_path : directory) {
    output.push_back(sub_path);
    if (is_directory(sub_path)) {
      list_directory_recursive(output, sub_path);
    }
  }
}

void Filesystem::exists(const y::string& path) const
{
  return is_file(path) || is_directory(path);
}
