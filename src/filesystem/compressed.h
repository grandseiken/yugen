#ifndef FILESYSTEM_COMPRESSED_H
#define FILESYSTEM_COMPRESSED_H

#include "filesystem.h"
#include <unordered_set>

// Implementation of Filesystem that wraps another (concrete) implementation,
// adding compression to a given set of file extensions.
// Will fall back to uncompressed files when loading if a compressed version
// does not exist.
class CompressedFilesystem : public Filesystem {
public:

  enum compression_algorithm {
    ZLIB,
    GZIP,
    BZIP2,
  };

  CompressedFilesystem(Filesystem& base, compression_algorithm algorithm);
  ~CompressedFilesystem() override {}

  // Signals that files of a given extension (without dot) should be compressed.
  // The set of extensions to be compressed should be completely specified
  // before Filesystem is used.
  // If no extensions are specified, all files will be compressed.
  void add_compressed_extension(const std::string& extension);

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

  std::string get_suffix() const;
  bool should_compress(const std::string& path) const;

  Filesystem& _base;
  compression_algorithm _algorithm;
  std::unordered_set<std::string> _extensions;

};

#endif
