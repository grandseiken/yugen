#include "filesystem.h"

#include <algorithm>

void Filesystem::list_pattern(y::string_vector& output,
                              const y::string& pattern) const
{
  struct local {
    static bool check_pattern_recursive(
        const y::string& path, y::size path_consume,
        const y::string& pattern, y::size star_pos)
    {
      // Check whether double (or more) star; find following non-star substring.
      bool recursive = false;
      y::size left_pos = star_pos + 1;
      y::size next_pos = left_pos < pattern.length() ?
          pattern.find_first_of('*', left_pos) : y::string::npos;

      while (next_pos == left_pos) {
        recursive = true;
        next_pos = ++left_pos < pattern.length() ?
            pattern.find_first_of('*', left_pos) : y::string::npos;
      }

      // If there's no following substring, just check the expansion matches.
      if (left_pos >= pattern.length()) {
        return recursive ||
            path.find_first_of('/', path_consume) == y::string::npos;
      }

      // Find that substring in path.
      y::string sub_pattern = pattern.substr(left_pos, next_pos - left_pos);
      y::size find = path.find(sub_pattern, path_consume);

      while (true) {
        if (find == y::string::npos) {
          return false;
        }
        // If it's single-star but the expansion contains a slash then it can't
        // match. Don't need to backtrack since subsequent matches will have
        // superstring expansions.
        y::string expansion = path.substr(path_consume, find - path_consume);
        if (!recursive && expansion.find_first_of('/') != y::string::npos) {
          return false;
        }

        if (find + sub_pattern.length() == path.length() ||
            check_pattern_recursive(path, find + sub_pattern.length(),
                                    pattern, next_pos)) {
          return true;
        }

        // Try the next match of that substring in the path.
        find = path.find(sub_pattern, find + 1);
      }
    }

    static bool check_pattern(const y::string& path, const y::string& pattern)
    {
      y::size star_pos = pattern.find_first_of('*');
      if (star_pos > 0 &&
          pattern.substr(0, star_pos) != path.substr(0, star_pos)) {
        return false;
      }

      return check_pattern_recursive(path, star_pos, pattern, star_pos);
    }
  };

  y::string cpattern;
  if (!canonicalise_path(cpattern, pattern)) {
    return;
  }
  y::size star_pos = cpattern.find_first_of('*');

  // Constant pattern is just a check for existence.
  if (star_pos == y::string::npos) {
    if (exists(cpattern)) {
      output.emplace_back(cpattern);
    }
    return;
  }

  y::size left = cpattern.find_last_of('/', star_pos);
  y::size right = cpattern.find_first_of('/', star_pos);

  y::string pattern_prefix = cpattern.substr(0, left);
  y::string_vector intermediate;

  // If we see a double star in the first path segment with stars, need to check
  // everything anyway.
  if (cpattern.find("**", star_pos) < right) {
    list_directory_recursive(intermediate, pattern_prefix);
    std::copy_if(
        intermediate.begin(), intermediate.end(), std::back_inserter(output),
        [&](const y::string& p){return local::check_pattern(p, cpattern);});
    return;
  }

  // Otherwise we can pick the subdirectories that actually match the path
  // segment and recurse.
  y::string pattern_rel = cpattern.substr(left + 1, right - left - 1);
  y::string pattern_postfix = cpattern.substr(right + 1);
  list_directory(intermediate, pattern_prefix);

  for (const y::string& path : intermediate) {
    y::string path_rel = path.substr(left + 1);
    if (!local::check_pattern(path_rel, pattern_rel)) {
      continue;
    }

    if (right == y::string::npos) {
      output.emplace_back(path);
    }
    else if (is_directory(path)) {
      list_pattern(
          output, pattern_prefix + '/' + path_rel + '/' + pattern_postfix);
    }
  }
}

void Filesystem::list_directory(y::string_vector& output,
                                const y::string& path) const
{
  y::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return;
  }

  y::string_vector intermediate;
  list_directory_internal(intermediate, cpath);

  y::size first = output.size();
  std::copy_if(
      intermediate.begin(), intermediate.end(), std::back_inserter(output),
      [&](const y::string& p){return exists(p);});
  std::sort(output.begin() + first, output.end());
}

void Filesystem::list_directory_recursive(y::string_vector& output,
                                          const y::string& path) const
{
  // Canonicalising is handled by list_directory.
  y::string_vector directory;
  list_directory(directory, path);

  for (const y::string& sub_path : directory) {
    output.emplace_back(sub_path);
    if (is_directory(sub_path)) {
      list_directory_recursive(output, sub_path);
    }
  }
}

bool Filesystem::exists(const y::string& path) const
{
  y::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  return is_file_internal(cpath) || is_directory_internal(cpath);
}

bool Filesystem::is_file(const y::string& path) const
{
  y::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  return is_file_internal(cpath);
}

bool Filesystem::is_directory(const y::string& path) const
{
  y::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  return is_directory_internal(cpath);
}

bool Filesystem::read_file(y::string& output,
                           const y::string& path) const
{
  y::string cpath;
  if (!canonicalise_path(cpath, path) || !is_file_internal(cpath)) {
    return false;
  }
  read_file_internal(output, cpath);
  return true;
}

bool Filesystem::read_file_with_includes(y::string& output,
                                         const y::string& path) const
{
  if (!read_file(output, path)) {
    return false;
  }
  static const y::string include_directive = "#include \"";
  static const y::size include_limit = 128;

  y::size include_count = 0;
  y::size include = output.find(include_directive);
  while (include != y::string::npos) {
    if (++include_count > include_limit) {
      std::cerr << "Include depth too deep in file " << path << std::endl;
      return false;
    }
    y::size begin = include + include_directive.length();
    y::size end = output.find_first_of('"', begin);
    if (end == y::string::npos) {
      break;
    }

    y::string include_filename = output.substr(begin, end - begin);
    if (include_filename[0] != '/') {
      y::string dir;
      dirname(dir, path);
      include_filename = dir + '/' + include_filename;
    }

    y::string after = output.substr(1 + end);
    output = output.substr(0, include);
    if (!read_file(output, include_filename)) {
      std::cerr << "Couldn't read file " << include_filename << std::endl;
      return false;
    }
    output += after;
    include = output.find(include_directive, include);
  }
  return true;
}

bool Filesystem::write_file(const y::string& data,
                            const y::string& path)
{
  y::string cpath;
  if (!canonicalise_path(cpath, path) || is_directory_internal(cpath)) {
    return false;
  }
  write_file_internal(data, cpath);
  return true;
}

bool Filesystem::basename(y::string& output, const y::string& path) const
{
  y::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  output = path.substr(path.find_last_of('/') + 1);
  return true;
}

bool Filesystem::dirname(y::string& output, const y::string& path) const
{
  y::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  output = path.substr(0, path.find_last_of('/'));
  return true;
}

bool Filesystem::extension(y::string& output, const y::string& path) const
{
  y::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  output = path.substr(path.find_last_of('.') + 1);
  return true;
}

bool Filesystem::barename(y::string& output, const y::string& path) const
{
  y::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  output = path.substr(0, path.find_last_of('.'));
  return true;
}

bool Filesystem::canonicalise_path(y::string& output,
                                   const y::string& path) const
{
  struct local {
    static bool process_segment(y::string_vector& split,
                                const y::string& segment)
    {
      if (segment == "..") {
        if (split.empty()) {
          return false;
        }
        split.erase(split.end() - 1);
      }
      else if (segment != "." && segment != "") {
        split.emplace_back(segment);
      }
      return true;
    }
  };

  y::string_vector split;
  y::size p = 0;
  y::size q = path.find_first_of('/');
  while (q != y::string::npos) {
    if (!local::process_segment(split, path.substr(p, q - p))) {
      return false;
    }
    p = 1 + q;
    q = p < path.length() ? path.find_first_of('/', p) : y::string::npos;
  }
  if (!local::process_segment(split, path.substr(p, q - p))) {
    return false;
  }

  for (const y::string& segment : split) {
    output += '/' + segment;
  }
  if (split.empty()) {
    output += '/';
  }
  return true;
}
