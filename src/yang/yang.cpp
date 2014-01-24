#include "../common/io.h"
#include "../common/math.h"
#include "../log.h"

#include "pipeline.h"
#include <boost/filesystem.hpp>
#include <fstream>

template<typename T>
using Fn = yang::Function<T>;

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

  yang::Context context;
  // TODO: test.
  // TODO: make sure injecting functions which take arguments by const& works.
  context.add_function(
      "foo", y::function<yang::int32_vec<2>(yang::int32_vec<2>)>(
          [](yang::int32_vec<2> a){return 2 * a;}));
  yang::Program program(context, path, contents);
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
  typedef Fn<yang::int32(yang::int32)> fn_t;
  auto h = instance.get_function<fn_t>("h");
  log_info("h(7): ", h(7));
  auto f = instance.get_function<fn_t>("f");
  instance.set_global("g", f);
  log_info("h(7): ", h(7));
  log_info("foo((2, 3)): ", instance.call<yang::int32_vec<2>>("foo", yang::int32_vec<2>(2, 3)));
  return 0;
}
