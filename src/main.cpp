#include <cstdio>

#define MAX_TOKENS 100
#define MAX_SOURCE_LENGTH (1024*1024)

enum class TokenType {
  kInteger,
  kOpPlus,
  kOpMinus,
  kEOF,
  kUnknown,
};

const char* token_name_table[] = {
  "kInteger",
  "kOpPlus",
  "kOpMinus",
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

bool Parse(Token* tokens) {
  size_t i = 0;
  if (tokens[i].type != TokenType::kInteger) {
    fprintf(stderr, "Error in Parse: kInteger is expected. actual %s\n", GetTokenName(tokens[i].type));
    return false;
  }

  printf(R"(.intel_syntax noprefix
.global main
main:
)");
  printf("  mov eax, %d\n", tokens[i].int_value);

  ++i;
  if (tokens[i].type == TokenType::kEOF) {
    goto eof;
  }

  while (tokens[i].type != TokenType::kEOF) {
    const auto op_type = tokens[i].type;

    ++i;
    if (tokens[i].type != TokenType::kInteger) {
      fprintf(stderr, "Error in Parse: kInteger is expected. actual %s\n", GetTokenName(tokens[i].type));
      return false;
    }

    const char* op_mnemonic;
    switch (op_type) {
      case TokenType::kOpPlus: op_mnemonic = "add"; break;
      case TokenType::kOpMinus: op_mnemonic = "sub"; break;
      default:
        fprintf(stderr, "Error in Parse: Unknown operator %s\n", GetTokenName(tokens[i].type));
        return false;
    }
    printf("  %s eax, %d\n", op_mnemonic, tokens[i].int_value);

    ++i;
  }

eof:
  if (tokens[i].type == TokenType::kEOF) {
    printf("  ret\n");
    return true;
  }

  fprintf(stderr, "Error in Parse: kEOF is expected. actual %s\n", GetTokenName(tokens[i].type));
  return false;
}

int main(void) {
  char src[MAX_SOURCE_LENGTH];
  fread(src, MAX_SOURCE_LENGTH, 1, stdin);

  SourceReader src_reader{src};

  Token tokens[MAX_TOKENS];
  if (auto result = Tokenize(src_reader, tokens); !result.success) {
    fprintf(stderr, "Tokenize failed at token %lu\n", result.value);
    return -1;
  }

  if (!Parse(tokens)) {
    fprintf(stderr, "Parse failed.\n");
    return -1;
  }
  return 0;
}
