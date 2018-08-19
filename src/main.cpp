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

template <typename Head, typename... Tail>
boost::format& BoostFormat(boost::format& format_string, Head head, Tail... tail) {
  return BoostFormat(format_string % head, tail...);
}

template <typename T>
boost::format& BoostFormat(boost::format& format_string, T head) {
  return format_string % head;
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

class CodeGenerateVisitor : public Visitor {
 public:
  CodeGenerateVisitor(std::vector<AssemblyLine>& code) : code_{code} {
  }

  void Visit(ASTNode* n) {
  }

  void Visit(CompoundStatement* stmt) {
    for (auto& n : stmt->statements) {
      n->Accept(this);
    }
    if (stmt->statements.empty()) {
      code_.push_back(AssemblyLine("  xor rax, rax"));
    }
  }

  void Visit(ExpressionStatement* stmt) {
    stmt->exp->Accept(this);
  }

  void Visit(EqualityExpression* exp) {
    exp->rhs->Accept(this);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this);
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

  void Visit(AdditiveExpression* exp) {
    exp->rhs->Accept(this);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this);
    code_.push_back(AssemblyLine("  pop rbx"));

    const char* op_mnemonic = "";
    if (exp->op == TokenType::kOpPlus) {
      op_mnemonic = "add";
    } else if (exp->op == TokenType::kOpMinus) {
      op_mnemonic = "sub";
    }
    code_.push_back(AssemblyLine("  %1% eax, ebx").Format(op_mnemonic));
  }

  void Visit(MultiplicativeExpression* exp) {
    exp->rhs->Accept(this);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this);
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

  void Visit(IntegerLiteral* exp) {
    code_.push_back(AssemblyLine("  mov eax, %1%").Format(exp->value));
  }

 private:
  std::vector<AssemblyLine>& code_;
};

class CodeGenerator {
 public:
  void Generate(std::shared_ptr<ASTNode> ast_root) {
    code_.push_back(AssemblyLine(".intel_syntax noprefix"));
    code_.push_back(AssemblyLine(".global main"));
    code_.push_back(AssemblyLine("main:"));

    CodeGenerateVisitor visitor{code_};
    ast_root->Accept(&visitor);

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
