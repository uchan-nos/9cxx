#include <cstdio>

#define MAX_TOKENS 100
#define MAX_SOURCE_LENGTH (1024*1024)

enum class TokenType {
  kInteger,
  kOpPlus,
  kOpMinus,
  kOpMult,
  kOpDiv,
  kOpEqual,
  kOpNotEqual,
  kLParen,
  kRParen,
  kLBrace,
  kRBrace,
  kSemicolon,
  kEOF,
  kUnknown,
};

const char* token_name_table[] = {
  "kInteger",
  "kOpPlus",
  "kOpMinus",
  "kOpMult",
  "kOpDiv",
  "kOpEqual",
  "kOpNotEqual",
  "kLParen",
  "kRParen",
  "kLBrace",
  "kRBrace",
  "kSemicolon",
  "kEOF",
  "kUnknown",
};

const char* GetTokenName(TokenType type) {
  return token_name_table[static_cast<int>(type)];
}

struct Token {
  TokenType type;
  int int_value;
};

constexpr bool IsSpace(char c) {
  return c == ' ' || c == '\t' || c == '\n';
}

template <typename T>
struct ReadResult {
  bool success;
  T value;
};

class SourceReader {
 public:
  SourceReader(const char* src) : src_{src}, read_pos_{src} {
  }

  bool Read(char expected) {
    if (*read_pos_ == expected) {
      ++read_pos_;
      return true;
    }
    return false;
  }

  ReadResult<int> ReadDigit() {
    if (*read_pos_ != '0' && '0' <= *read_pos_ && *read_pos_ <= '9') {
      int value = *read_pos_ - '0';
      ++read_pos_;
      return {true, value};
    }
    return {false, 0};
  }

  void SkipSpaces() {
    while (*read_pos_ != '\0' && IsSpace(*read_pos_)) {
      ++read_pos_;
    }
  }

 private:
  const char* src_;
  const char* read_pos_;
};

ReadResult<int> ReadInteger(SourceReader& reader) {
  ReadResult<int> digit = reader.ReadDigit();
  if (!digit.success) {
    return {false, 0};
  }

  int value = 0;
  while (digit.success) {
    value = value * 10 + digit.value;
    digit = reader.ReadDigit();
  }
  return {true, value};
}

Token ReadToken(SourceReader& reader) {
  if (reader.Read('+')) {
    return {TokenType::kOpPlus, 0};
  } else if (reader.Read('-')) {
    return {TokenType::kOpMinus, 0};
  } else if (reader.Read('*')) {
    return {TokenType::kOpMult, 0};
  } else if (reader.Read('/')) {
    return {TokenType::kOpDiv, 0};
  } else if (reader.Read('=')) {
    if (reader.Read('=')) {
      return {TokenType::kOpEqual, 0};
    } else {
      return {TokenType::kUnknown, 0};
    }
  } else if (reader.Read('!')) {
    if (reader.Read('=')) {
      return {TokenType::kOpNotEqual, 0};
    } else {
      return {TokenType::kUnknown, 0};
    }
  } else if (reader.Read('(')) {
    return {TokenType::kLParen, 0};
  } else if (reader.Read(')')) {
    return {TokenType::kRParen, 0};
  } else if (reader.Read('{')) {
    return {TokenType::kLBrace, 0};
  } else if (reader.Read('}')) {
    return {TokenType::kRBrace, 0};
  } else if (reader.Read(';')) {
    return {TokenType::kSemicolon, 0};
  } else if (auto result = ReadInteger(reader); result.success) {
    return {TokenType::kInteger, result.value};
  } else if (reader.Read('\0')) {
    return {TokenType::kEOF, 0};
  }
  return {TokenType::kUnknown, 0};
}

ReadResult<size_t> Tokenize(SourceReader& reader, Token* tokens) {
  for (size_t i = 0; i < MAX_TOKENS; ++i) {
    reader.SkipSpaces();
    Token token = ReadToken(reader);
    if (token.type == TokenType::kUnknown) {
      fprintf(stderr, "Error in Tokenize: token type is kUnknown.\n");
      return {false, i};
    }

    tokens[i] = token;
    if (token.type == TokenType::kEOF) {
      return {true, i + 1};
    }
  }
  fprintf(stderr, "Error in Tokenize: too many tokens.\n");
  return {false, MAX_TOKENS};
}

class TokenReader {
 public:
  TokenReader(const Token* tokens, size_t ntokens)
      : src_{tokens}, read_pos_{0}, size_{ntokens} {
  }

  const Token& Read() {
    const Token& token = src_[read_pos_];
    if (read_pos_ < size_) {
      ++read_pos_;
    }
    return token;
  }

  const Token& Current() const {
    return src_[read_pos_];
  }

  bool Read(TokenType expected) {
    if (src_[read_pos_].type == expected) {
      if (read_pos_ < size_) {
        ++read_pos_;
      }
      return true;
    }
    return false;
  }

 private:
  const Token* const src_;
  size_t read_pos_;
  size_t size_;
};

bool ReadExpr(TokenReader& reader);

bool ReadPrimaryExpr(TokenReader& reader) {
  if (reader.Read(TokenType::kLParen)) {
    if (!ReadExpr(reader)) {
      return false;
    }
    Token token = reader.Read();
    if (token.type != TokenType::kRParen) {
      fprintf(stderr, "Error in ReadPrimaryExpr: kRParen is expectd. actual %s\n", GetTokenName(token.type));
      return false;
    }
    return true;
  }

  Token token = reader.Read();
  if (token.type != TokenType::kInteger) {
    fprintf(stderr, "Error in ReadPrimaryExpr: kInteger is expected. actual %s\n", GetTokenName(token.type));
    return false;
  }

  printf("  mov eax, %d\n", token.int_value);
  return true;
}

bool ReadMultiplicativeExpr(TokenReader& reader) {
  if (!ReadPrimaryExpr(reader)) {
    return false;
  }

  while (true) {
    const char* op_mnemonic;
    if (reader.Read(TokenType::kOpMult)) {
      op_mnemonic = "mul";
    } else if (reader.Read(TokenType::kOpDiv)) {
      op_mnemonic = "div";
    } else {
      return true;
    }

    printf("  push rax\n");
    if (!ReadPrimaryExpr(reader)) {
      return false;
    }

    printf("  mov rbx, rax\n");
    printf("  pop rax\n");
    printf("  xor rdx,rdx\n  %s ebx\n", op_mnemonic);
  }
}

bool ReadAdditiveExpr(TokenReader& reader) {
  if (!ReadMultiplicativeExpr(reader)) {
    return false;
  }

  while (true) {
    const char* op_mnemonic;
    if (reader.Read(TokenType::kOpPlus)) {
      op_mnemonic = "add";
    } else if (reader.Read(TokenType::kOpMinus)) {
      op_mnemonic = "sub";
    } else {
      return true;
    }

    printf("  push rax\n");
    if (!ReadMultiplicativeExpr(reader)) {
      return false;
    }

    printf("  pop rbx\n");
    printf("  %s rbx, rax\n", op_mnemonic);
    printf("  mov rax, rbx\n");
  }
}

bool ReadEqualityExpr(TokenReader& reader) {
  if (!ReadAdditiveExpr(reader)) {
    return false;
  }

  while (true) {
    const char* op_mnemonic;
    if (reader.Read(TokenType::kOpEqual)) {
      op_mnemonic = "sete";
    } else if (reader.Read(TokenType::kOpNotEqual)) {
      op_mnemonic = "setne";
    } else {
      return true;
    }

    printf("  push rax\n");
    if (!ReadAdditiveExpr(reader)) {
      return false;
    }

    printf("  pop rbx\n  cmp rax, rbx\n  xor rax, rax\n  %s al\n", op_mnemonic);
  }
}

bool ReadExpr(TokenReader& reader) {
  return ReadEqualityExpr(reader);
}

bool ReadExpressionStatement(TokenReader& reader) {
  if (!ReadExpr(reader)) {
    return false;
  }

  Token token = reader.Read();
  if (token.type != TokenType::kSemicolon) {
    fprintf(stderr, "Error in ReadExpressionStatement: kSemicolon is expected. actual %s\n", GetTokenName(token.type));
    return false;
  }

  return true;
}

bool ReadStatement(TokenReader& reader) {
  printf("  xor rax, rax\n");
  if (reader.Read(TokenType::kLBrace)) {
    while (!reader.Read(TokenType::kRBrace)) {
      if (!ReadStatement(reader)) {
        return false;
      }
    }
    return true;
  }

  return ReadExpressionStatement(reader);
}

bool Parse(TokenReader& reader) {
  printf(R"(.intel_syntax noprefix
.global main
main:
)");

  if (!ReadStatement(reader)) {
    return false;
  }

  auto token = reader.Current();
  if (token.type == TokenType::kEOF) {
    printf("  ret\n");
    return true;
  }

  fprintf(stderr, "Error in Parse: kEOF is expected. actual %s\n", GetTokenName(token.type));
  return false;
}

int main(void) {
  char src[MAX_SOURCE_LENGTH];
  fread(src, MAX_SOURCE_LENGTH, 1, stdin);

  SourceReader src_reader{src};

  Token tokens[MAX_TOKENS];
  auto result = Tokenize(src_reader, tokens);
  if (!result.success) {
    fprintf(stderr, "Tokenize failed at token %lu\n", result.value);
    return -1;
  }

  TokenReader token_reader{tokens, result.value};

  if (!Parse(token_reader)) {
    fprintf(stderr, "Parse failed.\n");
    return -1;
  }
  return 0;
}
