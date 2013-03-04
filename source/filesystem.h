#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "common.h"

// TODO: Errors or exceptions?
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
  /*****/ void read_file(y::string& output,
                         const y::string& path) const;

  // Creates or overwrites the given file with the given contents.
  /*****/ void write_file(const y::string& data,
                          const y::string& path);

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
