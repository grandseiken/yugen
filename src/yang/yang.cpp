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

  log_info("Source:\n", program.print_ast());

  program.generate_ir();
  log_info("IR:\n", program.print_ir());

  program.optimise_ir();
  log_info("Optimised IR:\n", program.print_ir());

  return 0;
}
