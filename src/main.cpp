#include <cstdio>
#include <iostream>
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <boost/format.hpp>

#include "tokenizer.hpp"
#include "parser.hpp"

#define MAX_SOURCE_LENGTH (1024*1024)

template <typename T>
boost::format& BoostFormat(boost::format& format_string, T head) {
  return format_string % head;
}

template <typename Head, typename... Tail>
boost::format& BoostFormat(boost::format& format_string, Head head, Tail... tail) {
  return BoostFormat(format_string % head, tail...);
}

class AssemblyLine {
 public:
  AssemblyLine(const std::string& line) : line_{line} {
  }

  template <typename... Args>
  AssemblyLine& Format(Args... args) {
    auto fmt = boost::format(line_);
    line_ = BoostFormat(fmt, args...).str();
    return *this;
  }

  const std::string& ToString() const {
    return line_;
  }

 private:
  std::string line_;
};

enum class IdType {
  kUnknown,
  kLocalVariable,
  kGlobal,
};

struct IdInfo {
  IdType type;
  size_t rbp_offset; // [rbp - rbp_offset]
};

class CodeGenerateVisitor : public Visitor {
 public:
  CodeGenerateVisitor(std::vector<AssemblyLine>& code)
      : code_{code}, ids_{}, last_rbp_offset_{0} {
  }

  void Visit(CompoundStatement* stmt, bool lvalue) {
    if (stmt->statements.empty()) {
      code_.push_back(AssemblyLine("  xor rax, rax"));
      return;
    }

    code_.push_back(AssemblyLine("  mov rbp, rsp"));
    code_.push_back(AssemblyLine("  sub rsp, %1%"));
    size_t rsp_line = code_.size() - 1;

    for (auto& n : stmt->statements) {
      n->Accept(this, lvalue);
    }

    size_t stack_size = (last_rbp_offset_ + 15) & ~static_cast<size_t>(15);
    code_[rsp_line].Format(stack_size);
    code_.push_back(AssemblyLine("  add rsp, %1%").Format(stack_size));
  }

  void Visit(ExpressionStatement* stmt, bool lvalue) {
    stmt->exp->Accept(this, lvalue);
  }

  void Visit(AssignmentExpression* exp, bool lvalue) {
    if (auto n = std::dynamic_pointer_cast<Identifier>(exp->lhs)) {
      const auto& id_name = n->value;
      if (ids_.find(id_name) == ids_.end()) {
        last_rbp_offset_ += 8;
        ids_[id_name].type = IdType::kLocalVariable;
        ids_[id_name].rbp_offset = last_rbp_offset_;
      }
    }

    exp->rhs->Accept(this, false);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this, true);
    code_.push_back(AssemblyLine("  pop rbx"));

    code_.push_back(AssemblyLine("  mov [rax], rbx"));
    if (!lvalue) {
      code_.push_back(AssemblyLine("  mov rax, rbx"));
    }
  }

  void Visit(EqualityExpression* exp, bool lvalue) {
    exp->rhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  pop rbx"));

    const char* op_mnemonic = "";
    if (exp->op == TokenType::kOpEqual) {
      op_mnemonic = "sete";
    } else if (exp->op == TokenType::kOpNotEqual) {
      op_mnemonic = "setne";
    }
    code_.push_back(AssemblyLine("  cmp eax, ebx"));
    code_.push_back(AssemblyLine("  %1% bl").Format(op_mnemonic));
    code_.push_back(AssemblyLine("  xor rax, rax"));
    code_.push_back(AssemblyLine("  mov al, bl"));
  }

  void Visit(AdditiveExpression* exp, bool lvalue) {
    exp->rhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  pop rbx"));

    const char* op_mnemonic = "";
    if (exp->op == TokenType::kOpPlus) {
      op_mnemonic = "add";
    } else if (exp->op == TokenType::kOpMinus) {
      op_mnemonic = "sub";
    }
    code_.push_back(AssemblyLine("  %1% eax, ebx").Format(op_mnemonic));
  }

  void Visit(MultiplicativeExpression* exp, bool lvalue) {
    exp->rhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  pop rbx"));

    const char* op_mnemonic = "";
    if (exp->op == TokenType::kOpMult) {
      op_mnemonic = "mul";
    } else if (exp->op == TokenType::kOpDiv) {
      op_mnemonic = "div";
    }
    code_.push_back(AssemblyLine("  xor rdx, rdx"));
    code_.push_back(AssemblyLine("  %1% ebx").Format(op_mnemonic));
  }

  void Visit(FunctionCallExpression* exp, bool lvalue) {
    if (auto n = std::dynamic_pointer_cast<Identifier>(exp->name)) {
      const auto& id_name = n->value;
      if (ids_.find(id_name) == ids_.end()) {
        ids_[id_name].type = IdType::kGlobal;
      }
    }

    exp->name->Accept(this, true);
    code_.push_back(AssemblyLine("  call rax"));
  }

  void Visit(IntegerLiteral* exp, bool lvalue) {
    code_.push_back(AssemblyLine("  mov eax, %1%").Format(exp->value));
  }

  void Visit(Identifier* exp, bool lvalue) {
    const char* op_mnemonic = "";
    const auto& id_name = exp->value;

    if (lvalue) {
      op_mnemonic = "lea";
    } else {
      op_mnemonic = "mov";
    }

    if (ids_[id_name].type == IdType::kLocalVariable) {
      code_.push_back(AssemblyLine("  %1% rax, [rbp - %2%]").Format(
            op_mnemonic, ids_[id_name].rbp_offset));
    } else {
      code_.push_back(AssemblyLine("  mov rax, offset %1%").Format(id_name));
    }
  }

  void Visit(SimpleDeclaration* decl) {
    // pass
  }

 private:
  std::vector<AssemblyLine>& code_;
  std::map<std::string, IdInfo> ids_;
  size_t last_rbp_offset_;
};

class CodeGenerator {
 public:
  void Generate(std::shared_ptr<ASTNode> ast_root) {
    code_.push_back(AssemblyLine(".intel_syntax noprefix"));
    code_.push_back(AssemblyLine(".global main"));
    code_.push_back(AssemblyLine("main:"));

    CodeGenerateVisitor visitor{code_};
    ast_root->Accept(&visitor, false);

    code_.push_back(AssemblyLine("  ret"));
  }

  const std::vector<AssemblyLine>& GetCode() const {
    return code_;
  }

 private:
  std::vector<AssemblyLine> code_;
};

int main(void) {
  char src[MAX_SOURCE_LENGTH];
  fread(src, MAX_SOURCE_LENGTH, 1, stdin);

  SourceReader src_reader{src};

  std::vector<Token> tokens;
  auto result = Tokenize(src_reader, tokens);
  if (!result.success) {
    fprintf(stderr, "Tokenize failed at token %lu\n", result.value);
    for (size_t i = 0; i < result.value; ++i) {
      fprintf(stderr, " %s", GetTokenName(tokens[i].type));
    }
    fprintf(stderr, "\n");
    return -1;
  }

  TokenReader token_reader{tokens};
  Parser parser{token_reader};
  if (!parser.Parse() || !parser.GetAST()) {
    fprintf(stderr, "Parse error\n");
    return -1;
  }

  CodeGenerator generator;
  generator.Generate(parser.GetAST());
  for (auto& line : generator.GetCode()) {
    std::cout << line.ToString() << std::endl;
  }
  return 0;
}
