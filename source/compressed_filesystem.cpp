#include "compressed_filesystem.h"

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

CompressedFilesystem::CompressedFilesystem(
    Filesystem& base, compression_algorithm algorithm)
  : _base(base)
  , _algorithm(algorithm)
{
}

void CompressedFilesystem::add_compressed_extension(const y::string& extension)
{
  _extensions.insert(extension);
}

void CompressedFilesystem::list_directory_internal(y::string_vector& output,
                                                   const y::string& path) const
{
  y::string_vector temp;
  _base.list_directory(temp, path);

  y::set<y::string> set;
  for (const y::string& p : temp) {
    if (p.length() > get_suffix().length() &&
        p.substr(p.length() - get_suffix().length()) == get_suffix()) {
      y::string base = p.substr(0, p.length() - get_suffix().length());

      if (should_compress(base)) {
        set.insert(base);
      }
    }
    else {
      set.insert(p);
    }
  }

  for (const y::string& p : set) {
    output.push_back(p);
  }
}

bool CompressedFilesystem::is_file_internal(const y::string& path) const
{
  return _base.is_file(path) ||
      (should_compress(path) && _base.is_file(path + get_suffix()));
}

bool CompressedFilesystem::is_directory_internal(const y::string& path) const
{
  return _base.is_directory(path);
}

void CompressedFilesystem::read_file_internal(y::string& output,
                                              const y::string& path) const
{
  if (!should_compress(path) || !_base.is_file(path + get_suffix())) {
    _base.read_file(output, path);
    return;
  }

  y::string compressed;
  _base.read_file(compressed, path + get_suffix());
  y::sstream ss;
  ss << compressed;
  
  boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
  switch (_algorithm) {
    case ZLIB:
      in.push(boost::iostreams::zlib_decompressor());
      break;
    case GZIP:
      in.push(boost::iostreams::gzip_decompressor());
      break;
    case BZIP2:
      in.push(boost::iostreams::bzip2_decompressor());
      break;
    default: {}
  }
  in.push(ss);

  y::sstream decompressed;
  boost::iostreams::copy(in, decompressed);
  output += decompressed.str();
}

void CompressedFilesystem::write_file_internal(const y::string& data,
                                               const y::string& path)
{
  if (!should_compress(path)) {
    _base.write_file(data, path);
    return;
  }

  y::string compressed;
  boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
  switch (_algorithm) {
    case ZLIB:
      out.push(boost::iostreams::zlib_compressor());
      break;
    case GZIP:
      out.push(boost::iostreams::gzip_compressor());
      break;
    case BZIP2:
      out.push(boost::iostreams::bzip2_compressor());
      break;
    default: {}
  }
  out.push(boost::iostreams::back_inserter(compressed));
  boost::iostreams::copy(boost::make_iterator_range(data), out);
  
  _base.write_file(compressed, path + get_suffix());
}

y::string CompressedFilesystem::get_suffix() const
{
  return _algorithm == ZLIB ? ".z" :
         _algorithm == GZIP ? ".gzip" :
         _algorithm == BZIP2 ? ".bz2" : "";
}

bool CompressedFilesystem::should_compress(const y::string& path) const
{
  y::string ext;
  extension(ext, path);
  return _extensions.empty() || _extensions.count(ext);
}
