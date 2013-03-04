#include "physical_filesystem.h"
#include "common/int_types.h"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <fstream>
#include <boost/filesystem.hpp>

PhysicalFilesystem::PhysicalFilesystem(const y::string& root)
  : _root(boost::filesystem::absolute(boost::filesystem::path(root))
          .generic_string())
{
}

void PhysicalFilesystem::list_directory_internal(y::string_vector& output,
                                                 const y::string& path) const
{
  try {
    boost::filesystem::path root(_root);
    for (auto i = boost::filesystem::directory_iterator(root / path);
         i != boost::filesystem::directory_iterator(); ++i) {
      output.push_back(boost::filesystem::absolute(i->path())
                       .generic_string().substr(_root.length()));
    }
  }
  catch (const boost::filesystem::filesystem_error& e) {}
}

bool PhysicalFilesystem::is_file_internal(const y::string& path) const
{
  try {
    boost::filesystem::path root(_root);
    if (boost::filesystem::is_regular_file(root / path)) {
      return true;
    }
  }
  catch (const boost::filesystem::filesystem_error& e) {}
  return false;
}

bool PhysicalFilesystem::is_directory_internal(const y::string& path) const
{
  try {
    boost::filesystem::path root(_root);
    if (boost::filesystem::is_directory(root / path)) {
      return true;
    }
  }
  catch (const boost::filesystem::filesystem_error& e) {}
  return false;
}

void PhysicalFilesystem::read_file_internal(y::string& output,
                                            const y::string& path) const
{
  try {
    boost::filesystem::path root(_root);
    std::ifstream file((root / path).native(),
                       std::ios::in | std::ios::binary);

    y::size first = output.length();
    file.seekg(0, std::ios::end);
    output.resize(output.length() + file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&output[first], output.length());
  }
  catch (const boost::filesystem::filesystem_error& e) {}
  catch (const std::ifstream::failure& e) {}
}

void PhysicalFilesystem::write_file_internal(const y::string& data,
                                             const y::string& path)
{
  try {
    boost::filesystem::path root(_root);
    std::ofstream file((root / path).native(),
                       std::ios::out | std::ios::binary | std::ios::trunc);
    file << data;
  }
  catch (const boost::filesystem::filesystem_error& e) {}
  catch (const std::ofstream::failure& e) {}
}
