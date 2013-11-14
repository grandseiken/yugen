#ifndef FILESYSTEM__COMPRESSED_H
#define FILESYSTEM__COMPRESSED_H

#include "filesystem.h"

// Implementation of Filesystem that wraps another (concrete) implementation,
// adding compression to a given set of file extensions.
// Will fall back to uncompressed files when loading if a compressed version
// does not exist.
class CompressedFilesystem : public Filesystem, public y::no_copy {
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
  void add_compressed_extension(const y::string& extension);

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

  y::string get_suffix() const;
  bool should_compress(const y::string& path) const;

  Filesystem& _base;
  compression_algorithm _algorithm;
  y::set<y::string> _extensions;

};

#endif