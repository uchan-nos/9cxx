#include "tokenizer.hpp"

const std::set<std::string> kKeywords = {
  "char",
  "int",
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
  "kComma",
  "kSemicolon",
  "kKeyword",
  "kEOF",
};

const char* GetTokenName(TokenType type) {
  return token_name_table[static_cast<int>(type)];
}

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
    return {TokenType::kComma, 0};
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
