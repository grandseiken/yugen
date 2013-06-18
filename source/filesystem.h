#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "common.h"

class Filesystem {
public:

  Filesystem() {}
  virtual ~Filesystem() {}

  // Lists all files and directories matching pattern, in which * expands to any
  // substring not containing '/' and ** to any substring (i.e. recursively).
  // '..' and '.' are collapsed before expansion.
  /*****/ void list_pattern(y::string_vector& output,
                            const y::string& pattern) const;

  // Lists all files and subdirectories in a directory.
  /*****/ void list_directory(y::string_vector& output,
                              const y::string& path) const;

  // Lists all files and subdirectories under a directory.
  /*****/ void list_directory_recursive(y::string_vector& output,
                                        const y::string& path) const;

  // Returns true iff the given path exists.
  /*****/ bool exists(const y::string& path) const;

  // Returns true iff the given path is a file.
  /*****/ bool is_file(const y::string& path) const;

  // Returns true iff the given path is a directory.
  /*****/ bool is_directory(const y::string& path) const;

  // Reads the given file in as a string.
  /*****/ bool read_file(y::string& output,
                         const y::string& path) const;

  // Reads the given file in, doing textual substitution of #include directives.
  /*****/ bool read_file_with_includes(y::string& output,
                                       const y::string& path) const;

  // Creates or overwrites the given file with the given contents.
  /*****/ bool write_file(const y::string& data,
                          const y::string& path);

  // Returns the last component of a path.
  /*****/ bool basename(y::string& output, const y::string& path) const;

  // Returns all but the last component of a path.
  /*****/ bool dirname(y::string& output, const y::string& path) const;

  // Returns the extension of a path.
  /*****/ bool extension(y::string& output, const y::string& path) const;

  // Returns all but the extension of a path.
  /*****/ bool barename(y::string& output, const y::string& path) const;

protected:

  virtual void list_directory_internal(y::string_vector& output,
                                       const y::string& path) const = 0;

  virtual bool is_file_internal(const y::string& path) const = 0;

  virtual bool is_directory_internal(const y::string& path) const = 0;

  virtual void read_file_internal(y::string& output,
                                  const y::string& path) const = 0;

  virtual void write_file_internal(const y::string& data,
                                   const y::string& path) = 0;

private:

  // Build canonical version of path. Returns false if it's an invalid path.
  /*****/ bool canonicalise_path(y::string& output,
                                 const y::string& path) const;

};

#endif
