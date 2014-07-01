#ifndef FILESYSTEM_PHYSICAL_H
#define FILESYSTEM_PHYSICAL_H

#include "filesystem.h"

// Implementation of Filesystem that reads from local disk.
class PhysicalFilesystem : public Filesystem {
public:

  PhysicalFilesystem(const std::string& root);
  ~PhysicalFilesystem() override {}

protected:

  void list_directory_internal(std::vector<std::string>& output,
                               const std::string& path) const override;

  bool is_file_internal(const std::string& path) const override;

  bool is_directory_internal(const std::string& path) const override;

  void read_file_internal(std::string& output,
                          const std::string& path) const override;

  void write_file_internal(const std::string& data,
                           const std::string& path) override;

private:

  const std::string _root;

};

#endif
