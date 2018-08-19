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

std::vector<AssemblyLine> GenerateCode(std::shared_ptr<ASTNode> ast_root) {
  std::vector<AssemblyLine> code;
  code.push_back(AssemblyLine(".intel_syntax noprefix"));
  code.push_back(AssemblyLine(".global main"));
  code.push_back(AssemblyLine("main:"));

  if (auto n = std::dynamic_pointer_cast<IntegerLiteral>(ast_root)) {
    code.push_back(AssemblyLine("  mov eax, %1%").Format(n->GetValue()));
  }

  code.push_back(AssemblyLine("  ret"));
  return code;
}

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
  if (!parser.Parse()) {
    fprintf(stderr, "Parse error\n");
    return -1;
  }

  auto code = GenerateCode(parser.GetAST());
  for (auto& line : code) {
    std::cout << line.ToString() << std::endl;
  }
  return 0;
}
