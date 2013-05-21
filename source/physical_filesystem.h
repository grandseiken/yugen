#ifndef PHYSICAL_FILESYSTEM_H
#define PHYSICAL_FILESYSTEM_H

#include "filesystem.h"

// Implementation of Filesystem that reads from local disk.
// TODO: wrapper implementation which gzips certain filetypes,
// or maybe do that automatically in save.h.
class PhysicalFilesystem : public Filesystem {
public:

  PhysicalFilesystem(const y::string& root);
  ~PhysicalFilesystem() override {}

protected:

  void list_directory_internal(y::string_vector& output,
                               const y::string& path) const override;

  bool is_file_internal(const y::string& path) const override;

  bool is_directory_internal(const y::string& path) const override;

  void read_file_internal(y::string& output,
                          const y::string& path) const override;

  void write_file_internal(const y::string& data,
                           const y::string& path) override;

private:

  const y::string _root;

};

#endif
