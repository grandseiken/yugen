#include "physical.h"
#include "../log.h"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <fstream>
#include <boost/filesystem.hpp>

namespace y {
  typedef std::ifstream ifstream;
}

PhysicalFilesystem::PhysicalFilesystem(const std::string& root)
  : _root(boost::filesystem::absolute(boost::filesystem::path(root))
          .generic_string())
{
}

void PhysicalFilesystem::list_directory_internal(std::vector<std::string>& output,
                                                 const std::string& path) const
{
  try {
    boost::filesystem::path root(_root);
    for (auto i = boost::filesystem::directory_iterator(root / path);
         i != boost::filesystem::directory_iterator(); ++i) {
      output.emplace_back(boost::filesystem::absolute(i->path())
                          .generic_string().substr(_root.length()));
    }
  }
  catch (const boost::filesystem::filesystem_error&) {}
}

bool PhysicalFilesystem::is_file_internal(const std::string& path) const
{
  try {
    boost::filesystem::path root(_root);
    if (boost::filesystem::is_regular_file(root / path)) {
      return true;
    }
  }
  catch (const boost::filesystem::filesystem_error&) {}
  return false;
}

bool PhysicalFilesystem::is_directory_internal(const std::string& path) const
{
  try {
    boost::filesystem::path root(_root);
    if (boost::filesystem::is_directory(root / path)) {
      return true;
    }
  }
  catch (const boost::filesystem::filesystem_error&) {}
  return false;
}

void PhysicalFilesystem::read_file_internal(std::string& output,
                                            const std::string& path) const
{
  try {
    boost::filesystem::path root(_root);
    std::ifstream file((root / path).native(),
                       std::ios::in | std::ios::binary);

    std::size_t first = output.length();
    file.seekg(0, std::ios::end);
    output.resize(output.length() + file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&output[first], output.length());
  }
  catch (const boost::filesystem::filesystem_error& e) {
    log_err("Read of ", path, " failed: ", e.what());
  }
  catch (const std::ofstream::failure& e) {
    log_err("Read of ", path, " failed: ", e.what());
  }
}

void PhysicalFilesystem::write_file_internal(const std::string& data,
                                             const std::string& path)
{
  try {
    boost::filesystem::path root(_root);
    std::ofstream file((root / path).native(),
                       std::ios::out | std::ios::binary | std::ios::trunc);
    file << data;
  }
  catch (const boost::filesystem::filesystem_error& e) {
    log_err("Write of ", path, " failed: ", e.what());
  }
  catch (const std::ofstream::failure& e) {
    log_err("Write of ", path, " failed: ", e.what());
  }
}
