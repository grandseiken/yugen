#ifndef FILESYSTEM_FILESYSTEM_H
#define FILESYSTEM_FILESYSTEM_H

#include <string>
#include <vector>

class Filesystem {
public:

  Filesystem() {}
  virtual ~Filesystem() {}

  // Lists all files and directories matching pattern, in which * expands to any
  // substring not containing '/' and ** to any substring (i.e. recursively).
  // '..' and '.' are collapsed before expansion.
  /*****/ void list_pattern(std::vector<std::string>& output,
                            const std::string& pattern) const;

  // Lists all files and subdirectories in a directory.
  /*****/ void list_directory(std::vector<std::string>& output,
                              const std::string& path) const;

  // Lists all files and subdirectories under a directory.
  /*****/ void list_directory_recursive(std::vector<std::string>& output,
                                        const std::string& path) const;

  // Returns true iff the given path exists.
  /*****/ bool exists(const std::string& path) const;

  // Returns true iff the given path is a file.
  /*****/ bool is_file(const std::string& path) const;

  // Returns true iff the given path is a directory.
  /*****/ bool is_directory(const std::string& path) const;

  // Reads the given file in as a string.
  /*****/ bool read_file(std::string& output,
                         const std::string& path) const;

  // Reads the given file in, doing textual substitution of #include directives.
  /*****/ bool read_file_with_includes(std::string& output,
                                       const std::string& path) const;

  // Creates or overwrites the given file with the given contents.
  /*****/ bool write_file(const std::string& data,
                          const std::string& path);

  // Returns the last component of a path.
  /*****/ bool basename(std::string& output, const std::string& path) const;

  // Returns all but the last component of a path.
  /*****/ bool dirname(std::string& output, const std::string& path) const;

  // Returns the extension of a path (without dot).
  /*****/ bool extension(std::string& output, const std::string& path) const;

  // Returns all but the extension of a path (without dot).
  /*****/ bool barename(std::string& output, const std::string& path) const;

protected:

  virtual void list_directory_internal(std::vector<std::string>& output,
                                       const std::string& path) const = 0;

  virtual bool is_file_internal(const std::string& path) const = 0;

  virtual bool is_directory_internal(const std::string& path) const = 0;

  virtual void read_file_internal(std::string& output,
                                  const std::string& path) const = 0;

  virtual void write_file_internal(const std::string& data,
                                   const std::string& path) = 0;

private:

  // Build canonical version of path. Returns false if it's an invalid path.
  /*****/ bool canonicalise_path(std::string& output,
                                 const std::string& path) const;

};

#endif
