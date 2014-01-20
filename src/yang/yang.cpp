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
  log_info("h(7): ", instance.call<yang::int32>("h", 7));
  instance.set_global(
      "g", instance.get_function<yang::Function<yang::int32, yang::int32>>("f"));
  log_info("h(7): ", instance.call<yang::int32>("h", 7));
  typedef yang::Function<yang::Function<yang::Function<yang::int32>>> ft;
  auto q = instance.get_function<ft>("q");
  log_info("q()()(): ", q.call().call().call());
  auto r = instance.get_function<ft>("r");
  log_info("r()()(): ", r.call().call().call());
  return 0;
}
