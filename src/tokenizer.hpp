#pragma once

#include <string>
#include <set>
#include <vector>

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
  kComma,
  kSemicolon,
  kKeyword,
  kEOF,
};

extern const char* token_name_table[];

const char* GetTokenName(TokenType type);

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

ReadResult<int> ReadInteger(SourceReader& reader);
ReadResult<std::string> ReadId(SourceReader& reader);
Token ReadToken(SourceReader& reader);
ReadResult<size_t> Tokenize(SourceReader& reader, std::vector<Token>& tokens);
