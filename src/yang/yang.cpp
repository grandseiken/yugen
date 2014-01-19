#include "../common/io.h"
#include "../common/math.h"
#include "../log.h"

#include "pipeline.h"
#include <boost/filesystem.hpp>
#include <fstream>

y::int32 main(y::int32 argc, char** argv)
{
  if (argc < 2) {
    log_err("No input file specified");
    return 1;
  }

  y::string path = argv[1];
  if (!boost::filesystem::is_regular_file(path)) {
    log_err(path, " is not a file");
    return 1;
  }

  y::string contents;
  try {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    file.seekg(0, std::ios::end);
    contents.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&contents[0], contents.length());
  }
  catch (const std::ofstream::failure& e) {
    log_err("Read of ", path, " failed");
    return 1;
  }

  yang::Program program(path, contents);
  if (!program.success()) {
    return 1;
  }
  for (const auto& pair : program.get_globals()) {
    if (pair.second.is_exported()) {
      log_info("global [", pair.second.string(), "] ", pair.first);
    }
  }
  for (const auto& pair : program.get_functions()) {
    log_info("function [", pair.second.string(), "] ", pair.first);
  }

  log_info("Source:\n", program.print_ast());
  log_info("IR:\n", program.print_ir());

  // TODO: test.
  yang::Instance instance(program);
  log_info("value of global `foo`: ", instance.get_global<yang::int32>("foo"));
  instance.set_global<yang::int32>("foo", 9);
  log_info("value of global `foo`: ", instance.get_global<yang::int32>("foo"));
  log_info("value of global `baz`: ",
           instance.get_global<yang::int32_vec<2>>("baz"));
  instance.set_global("baz", yang::int32_vec<2>(14, 15));
  log_info("value of global `baz`: ",
           instance.get_global<yang::int32_vec<2>>("baz"));
  return 0;
}
