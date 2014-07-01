#include "filesystem.h"
#include "../common.h"
#include "../log.h"
#include <functional>

namespace {

bool check_pattern_recursive(
    const std::string& path, std::size_t path_consume,
    const std::string& pattern, std::size_t star_pos)
{
  // Check whether double (or more) star; find following non-star substring.
  bool recursive = false;
  std::size_t left_pos = star_pos + 1;
  std::size_t next_pos = left_pos < pattern.length() ?
      pattern.find_first_of('*', left_pos) : std::string::npos;

  while (next_pos == left_pos) {
    recursive = true;
    next_pos = ++left_pos < pattern.length() ?
        pattern.find_first_of('*', left_pos) : std::string::npos;
  }

  // If there's no following substring, just check the expansion matches.
  if (left_pos >= pattern.length()) {
    return recursive ||
        path.find_first_of('/', path_consume) == std::string::npos;
  }

  // Find that substring in path.
  std::string sub_pattern = pattern.substr(left_pos, next_pos - left_pos);
  std::size_t find = path.find(sub_pattern, path_consume);

  while (true) {
    if (find == std::string::npos) {
      return false;
    }
    // If it's single-star but the expansion contains a slash then it can't
    // match. Don't need to backtrack since subsequent matches will have
    // superstring expansions.
    std::string expansion = path.substr(path_consume, find - path_consume);
    if (!recursive && expansion.find_first_of('/') != std::string::npos) {
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

// End anonymous namespace.
}

void Filesystem::list_pattern(std::vector<std::string>& output,
                              const std::string& pattern) const
{
  auto check_pattern = [&](const std::string& path, const std::string& pattern)
  {
    std::size_t star_pos = pattern.find_first_of('*');
    if (star_pos > 0 &&
        pattern.substr(0, star_pos) != path.substr(0, star_pos)) {
      return false;
    }

    return check_pattern_recursive(path, star_pos, pattern, star_pos);
  };

  std::string cpattern;
  if (!canonicalise_path(cpattern, pattern)) {
    return;
  }
  std::size_t star_pos = cpattern.find_first_of('*');

  // Constant pattern is just a check for existence.
  if (star_pos == std::string::npos) {
    if (exists(cpattern)) {
      output.emplace_back(cpattern);
    }
    return;
  }

  std::size_t left = cpattern.find_last_of('/', star_pos);
  std::size_t right = cpattern.find_first_of('/', star_pos);

  std::string pattern_prefix = cpattern.substr(0, left);
  std::vector<std::string> intermediate;

  // If we see a double star in the first path segment with stars, need to check
  // everything anyway.
  if (cpattern.find("**", star_pos) < right) {
    list_directory_recursive(intermediate, pattern_prefix);
    std::copy_if(
        intermediate.begin(), intermediate.end(), std::back_inserter(output),
        [&](const std::string& p){return check_pattern(p, cpattern);});
    return;
  }

  // Otherwise we can pick the subdirectories that actually match the path
  // segment and recurse.
  std::string pattern_rel = cpattern.substr(left + 1, right - left - 1);
  std::string pattern_postfix = cpattern.substr(right + 1);
  list_directory(intermediate, pattern_prefix);

  for (const std::string& path : intermediate) {
    std::string path_rel = path.substr(left + 1);
    if (!check_pattern(path_rel, pattern_rel)) {
      continue;
    }

    if (right == std::string::npos) {
      output.emplace_back(path);
    }
    else if (is_directory(path)) {
      list_pattern(
          output, pattern_prefix + '/' + path_rel + '/' + pattern_postfix);
    }
  }
}

void Filesystem::list_directory(std::vector<std::string>& output,
                                const std::string& path) const
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return;
  }

  std::vector<std::string> intermediate;
  list_directory_internal(intermediate, cpath);

  std::size_t first = output.size();
  std::copy_if(
      intermediate.begin(), intermediate.end(), std::back_inserter(output),
      [&](const std::string& p){return exists(p);});
  std::sort(output.begin() + first, output.end());
}

void Filesystem::list_directory_recursive(std::vector<std::string>& output,
                                          const std::string& path) const
{
  // Canonicalising is handled by list_directory.
  std::vector<std::string> directory;
  list_directory(directory, path);

  for (const std::string& sub_path : directory) {
    output.emplace_back(sub_path);
    if (is_directory(sub_path)) {
      list_directory_recursive(output, sub_path);
    }
  }
}

bool Filesystem::exists(const std::string& path) const
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  return is_file_internal(cpath) || is_directory_internal(cpath);
}

bool Filesystem::is_file(const std::string& path) const
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  return is_file_internal(cpath);
}

bool Filesystem::is_directory(const std::string& path) const
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  return is_directory_internal(cpath);
}

bool Filesystem::read_file(std::string& output,
                           const std::string& path) const
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    log_err("Couldn't read file ", path, ", invalid path");
    return false;
  }
  if (!is_file_internal(cpath)) {
    log_err("Couldn't read file ", path);
    return false;
  }
  read_file_internal(output, cpath);
  return true;
}

bool Filesystem::read_file_with_includes(std::string& output,
                                         const std::string& path) const
{
  if (!read_file(output, path)) {
    return false;
  }
  static const std::string include_directive = "#include \"";
  static const std::size_t include_limit = 128;

  std::size_t include_count = 0;
  std::size_t include = output.find(include_directive);
  while (include != std::string::npos) {
    if (++include_count > include_limit) {
      log_err("Include depth too deep in file ", path);
      return false;
    }
    std::size_t begin = include + include_directive.length();
    std::size_t end = output.find_first_of('"', begin);
    if (end == std::string::npos) {
      break;
    }

    std::string include_filename = output.substr(begin, end - begin);
    if (include_filename[0] != '/') {
      std::string dir;
      dirname(dir, path);
      include_filename = dir + '/' + include_filename;
    }

    std::string after = output.substr(1 + end);
    output = output.substr(0, include);
    if (!read_file(output, include_filename)) {
      log_err("Couldn't read included file ", include_filename);
      return false;
    }
    output += after;
    include = output.find(include_directive, include);
  }
  return true;
}

bool Filesystem::write_file(const std::string& data,
                            const std::string& path)
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    log_err("Couldn't write file ", path, ", invalid path");
    return false;
  }
  if (is_directory_internal(cpath)) {
    log_err("Couldn't write file ", path, ", is a directory");
    return false;
  }
  write_file_internal(data, cpath);
  return true;
}

bool Filesystem::basename(std::string& output, const std::string& path) const
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  output = path.substr(path.find_last_of('/') + 1);
  return true;
}

bool Filesystem::dirname(std::string& output, const std::string& path) const
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  output = path.substr(0, path.find_last_of('/'));
  return true;
}

bool Filesystem::extension(std::string& output, const std::string& path) const
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  output = path.substr(path.find_last_of('.') + 1);
  return true;
}

bool Filesystem::barename(std::string& output, const std::string& path) const
{
  std::string cpath;
  if (!canonicalise_path(cpath, path)) {
    return false;
  }
  output = path.substr(0, path.find_last_of('.'));
  return true;
}

bool Filesystem::canonicalise_path(std::string& output,
                                   const std::string& path) const
{
  auto process_segment = [&](
      std::vector<std::string>& split, const std::string& segment)
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
  };

  std::vector<std::string> split;
  std::size_t p = 0;
  std::size_t q = path.find_first_of('/');
  while (q != std::string::npos) {
    if (!process_segment(split, path.substr(p, q - p))) {
      return false;
    }
    p = 1 + q;
    q = p < path.length() ? path.find_first_of('/', p) : std::string::npos;
  }
  if (!process_segment(split, path.substr(p, q - p))) {
    return false;
  }

  for (const std::string& segment : split) {
    output += '/' + segment;
  }
  if (split.empty()) {
    output += '/';
  }
  return true;
}
