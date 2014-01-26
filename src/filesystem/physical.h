#ifndef FILESYSTEM_PHYSICAL_H
#define FILESYSTEM_PHYSICAL_H

#include "filesystem.h"

// Implementation of Filesystem that reads from local disk.
class PhysicalFilesystem : public Filesystem, public y::no_copy {
public:

  PhysicalFilesystem(const y::string& root);
  ~PhysicalFilesystem() override {}

protected:

  void list_directory_internal(y::vector<y::string>& output,
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
