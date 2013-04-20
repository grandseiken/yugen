#ifndef PHYSICAL_FILESYSTEM_H
#define PHYSICAL_FILESYSTEM_H

#include "filesystem.h"

// Implementation of Filesystem that reads from local disk.
class PhysicalFilesystem : public Filesystem {
public:

  PhysicalFilesystem(const y::string& root);
  virtual ~PhysicalFilesystem() {}

protected:

  virtual void list_directory_internal(y::string_vector& output,
                                       const y::string& path) const;

  virtual bool is_file_internal(const y::string& path) const;

  virtual bool is_directory_internal(const y::string& path) const;

  virtual void read_file_internal(y::string& output,
                                  const y::string& path) const;

  virtual void write_file_internal(const y::string& data,
                                   const y::string& path);

private:

  const y::string _root;

};

#endif
