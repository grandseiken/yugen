#include "static.h"
#include "../log.h"

Type StaticChecker::visit(const Node& node, const result_list& results)
{
  log_info("visiting (", results.size(), " children)");
}
