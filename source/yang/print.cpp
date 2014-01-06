#include "print.h"

AstPrinter::AstPrinter()
  : _indent(0)
{
}

void AstPrinter::preorder(const Node& node)
{
  switch (node.type) {
    case Node::BLOCK:
      ++_indent;
      break;

    default: {}
  }
}

void AstPrinter::infix(const Node& node, const result_list& results)
{
  (void)node;
  (void)results;
}

y::string AstPrinter::visit(const Node& node, const result_list& results)
{
  y::string s = Node::op_string(node.type);
  y::string type_id = node.string_value.length() ? " " + node.string_value : "";

  switch (node.type) {
    case Node::TYPE_VOID:
      return "void" + type_id;
    case Node::TYPE_INT:
      return "int" +
          (node.int_value > 1 ? y::to_string(node.int_value) : "") + type_id;
    case Node::TYPE_WORLD:
      return "world" +
          (node.int_value > 1 ? y::to_string(node.int_value) : "") + type_id;
    case Node::TYPE_FUNCTION:
    {
      y::string r = results[0] + "(";
      for (y::size i = 1; i < results.size(); ++i) {
        if (i > 1) {
          r += ", ";
        }
        r += results[i];
      }
      return r + ")" + type_id;
    }

    case Node::PROGRAM:
    {
      y::string s = "";
      for (const y::string r : results) {
        s += r + '\n';
      }
      return s;
    }
    case Node::FUNCTION:
      return (node.int_value ? "export " : "") +
          node.string_value + " " + results[0] + "\n" + results[1];
    case Node::GLOBAL:
      return (node.int_value ? "export " : "") +
          y::string("global\n") + results[0];

    case Node::BLOCK:
    {
      --_indent;
      y::string s = "";
      for (y::size i = 0; i < results.size(); ++i) {
        s += results[i];
      }
      return indent() + "{\n" + s + indent() + "}\n";
    }
    case Node::EMPTY_STMT:
      return indent() + ";\n";
    case Node::EXPR_STMT:
      return indent() + results[0] + ";\n";
    case Node::RETURN_STMT:
      return indent() + "return " + results[0] + ";\n";
    case Node::IF_STMT:
    {
      y::string s = indent() + "if (" + results[0] + ")\n" + results[1];
      if (results.size() > 2) {
        s += indent() + "else\n" + results[2];
      }
      return s;
    }
    case Node::FOR_STMT:
      return indent() +
          "for (" + results[0] + "; " + results[1] + "; " + results[2] + ")\n" +
          results[3];
    case Node::DO_WHILE_STMT:
      return indent() + "do\n" + results[0] +
          indent() +"while (" + results[1] + ");\n";
    case Node::BREAK_STMT:
      return indent() + "break;\n";
    case Node::CONTINUE_STMT:
      return indent() + "continue;\n";

    case Node::IDENTIFIER:
      return node.string_value;
    case Node::INT_LITERAL:
      return y::to_string(node.int_value);
    case Node::WORLD_LITERAL:
      return y::to_string(node.world_value);

    case Node::TERNARY:
      return "(" + results[0] + " ? " + results[1] + " : " + results[2] + ")";
    case Node::CALL:
    {
      y::string s = "(" + results[0] + "(";
      for (y::size i = 1; i < results.size(); ++i) {
        if (i > 1) {
          s += ", ";
        }
        s += results[i];
      }
      return s + "))";
    }

    case Node::LOGICAL_OR:
    case Node::LOGICAL_AND:
    case Node::BITWISE_OR:
    case Node::BITWISE_AND:
    case Node::BITWISE_XOR:
    case Node::BITWISE_LSHIFT:
    case Node::BITWISE_RSHIFT:
    case Node::POW:
    case Node::MOD:
    case Node::ADD:
    case Node::SUB:
    case Node::MUL:
    case Node::DIV:
    case Node::EQ:
    case Node::NE:
    case Node::GE:
    case Node::LE:
    case Node::GT:
    case Node::LT:
      return "(" + results[0] + " " + s + " " + results[1] + ")";

    case Node::FOLD_LOGICAL_OR:
    case Node::FOLD_LOGICAL_AND:
    case Node::FOLD_BITWISE_OR:
    case Node::FOLD_BITWISE_AND:
    case Node::FOLD_BITWISE_XOR:
    case Node::FOLD_BITWISE_LSHIFT:
    case Node::FOLD_BITWISE_RSHIFT:
    case Node::FOLD_POW:
    case Node::FOLD_MOD:
    case Node::FOLD_ADD:
    case Node::FOLD_SUB:
    case Node::FOLD_MUL:
    case Node::FOLD_DIV:
    case Node::FOLD_EQ:
    case Node::FOLD_NE:
    case Node::FOLD_GE:
    case Node::FOLD_LE:
    case Node::FOLD_GT:
    case Node::FOLD_LT:
      return "($" + results[0] + s + ")";

    case Node::LOGICAL_NEGATION:
    case Node::BITWISE_NEGATION:
    case Node::ARITHMETIC_NEGATION:
      return "(" + s + results[0] + ")";

    case Node::ASSIGN:
      return "(" + node.string_value + " = " + results[0] + ")";
    case Node::ASSIGN_VAR:
      return "(var " + node.string_value + " = " + results[0] + ")";
    case Node::ASSIGN_CONST:
      return "(const " + node.string_value + " = " + results[0] + ")";

    case Node::INT_CAST:
      return "[" + results[0] + "]";
    case Node::WORLD_CAST:
      return "(" + results[0] + ".)";

    case Node::VECTOR_CONSTRUCT:
    {
      y::string output = "";
      for (const y::string& s : results) {
        if (output.length()) {
          output += ", ";
        }
        output += s;
      }
      return "(" + output + ")";
    }
    case Node::VECTOR_INDEX:
      return "(" + results[0] + "[" + results[1] + "])";

    default:
      return "";
  }
}

y::string AstPrinter::indent() const
{
  return y::string(2 * _indent, ' ');
}
