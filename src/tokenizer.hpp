enum class TokenType {
  kUnknown,
  kInteger,
  kId,
  kOpPlus,
  kOpMinus,
  kOpMult,
  kOpDiv,
  kOpEqual,
  kOpNotEqual,
  kOpAssign,
  kLParen,
  kRParen,
  kLBrace,
  kRBrace,
  kColon,
  kSemicolon,
  kKeyword,
  kEOF,
};

const char* token_name_table[] = {
  "kUnknown",
  "kInteger",
  "kId",
  "kOpPlus",
  "kOpMinus",
  "kOpMult",
  "kOpDiv",
  "kOpEqual",
  "kOpNotEqual",
  "kOpAssign",
  "kLParen",
  "kRParen",
  "kLBrace",
  "kRBrace",
  "kColon",
  "kSemicolon",
  "kKeyword",
  "kEOF",
};

const char* GetTokenName(TokenType type) {
  return token_name_table[static_cast<int>(type)];
}

struct Token {
  TokenType type;
  int int_value;
  std::string string_value;
};

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
    if (isdigit(*read_pos_)) {
      int value = *read_pos_ - '0';
      ++read_pos_;
      return {true, value};
    }
    return {false, 0};
  }

  ReadResult<char> ReadAlphaUnder() {
    if (*read_pos_ == '_' || isalpha(*read_pos_)) {
      char value = *read_pos_;
      ++read_pos_;
      return {true, value};
    }
    return {false, 0};
  }

  void SkipSpaces() {
    while (*read_pos_ != '\0' && isspace(*read_pos_)) {
      ++read_pos_;
    }
  }

  char Current() const {
    return *read_pos_;
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

ReadResult<std::string> ReadId(SourceReader& reader) {
  ReadResult<char> ch = reader.ReadAlphaUnder();
  if (!ch.success) {
    return {false};
  }

  std::string value;
  value += ch.value;
  while (true) {
    if (ch = reader.ReadAlphaUnder(); ch.success) {
      value += ch.value;
    } else if (auto digit = reader.ReadDigit(); digit.success) {
      value += digit.value + '0';
    } else {
      break;
    }
  }
  return {true, value};
}

const std::set<std::string> kKeywords = {
  "char",
  "int",
};

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
      return {TokenType::kOpAssign, 0};
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
  } else if (reader.Read(',')) {
    return {TokenType::kColon, 0};
  } else if (reader.Read(';')) {
    return {TokenType::kSemicolon, 0};
  } else if (auto result = ReadInteger(reader); result.success) {
    return {TokenType::kInteger, result.value};
  } else if (auto result = ReadId(reader); result.success) {
    if (kKeywords.find(result.value) != kKeywords.end()) {
      return {TokenType::kKeyword, 0, result.value};
    }
    return {TokenType::kId, 0, result.value};
  } else if (reader.Read('\0')) {
    return {TokenType::kEOF, 0};
  }
  return {TokenType::kUnknown, 0};
}

ReadResult<size_t> Tokenize(SourceReader& reader, std::vector<Token>& tokens) {
  while(true) {
    reader.SkipSpaces();
    Token token = ReadToken(reader);
    if (token.type == TokenType::kUnknown) {
      fprintf(stderr, "Error in Tokenize: token type is kUnknown at '%c'\n", reader.Current());
      return {false, tokens.size()};
    }

    tokens.push_back(token);
    if (token.type == TokenType::kEOF) {
      return {true, tokens.size()};
    }
  }
}
