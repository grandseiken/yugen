#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "common.h"

// TODO: Errors or exceptions?
class Filesystem {
public:

  // Lists all files and directories matching pattern, in which * expands to any
  // substring not containing '/' and ** to any substring (i.e. recursively).
  /*****/ void list_pattern(y::string_vector& output,
                            const y::string& pattern) const;

  // Lists all files and subdirectories in a directory.
  virtual void list_directory(y::string_vector& output,
                              const y::string& path) const = 0;

  // Lists all files and subdirectories under a directory.
  /*****/ void list_directory_recursive(y::string_vector& output,
                                        const y::string& path) const;
        
  // Returns true iff the given path exists.
  /*****/ bool exists(const y::string& path) const;

  // Returns true iff the given path is a file.
  virtual bool is_file(const y::string& path) const = 0;

  // Returns true iff the given path is a directory.
  virtual bool is_directory(const y::string& path) const = 0;

  // Reads the given file in as a string.
  virtual void read_file(y::string& output,
                         const y::string& path) const = 0;

  // Creates or overwrites the given file with the given contents.
  virtual void write_file(const y::string& data,
                          const y::string& path) const = 0;

};

#endif
