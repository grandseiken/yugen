#include "pipeline.h"

#include "ast.h"
#include "static.h"
#include "../log.h"
#include "../../gen/yang/yang.y.h"

int yyparse();

y::unique<Node> parse_yang_ast(const y::string& contents)
{
  ParseGlobals::lexer_input_contents = &contents;
  ParseGlobals::lexer_input_offset = 0;
  ParseGlobals::errors.clear();

  yyparse();
  for (const y::string& s : ParseGlobals::errors) {
    log_err(s);
  }
  if (ParseGlobals::errors.size()) {
    return y::null;
  }

  StaticChecker checker;
  checker.walk(*ParseGlobals::parser_output);
  if (checker.errors()) {
    return y::null;
  }
  return y::move_unique(ParseGlobals::parser_output);
}

const y::string* ParseGlobals::lexer_input_contents = y::null;
y::size ParseGlobals::lexer_input_offset = 0;

Node* ParseGlobals::parser_output = y::null;
y::vector<y::string> ParseGlobals::errors;
