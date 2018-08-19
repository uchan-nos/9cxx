class TokenReader {
 public:
  TokenReader(const std::vector<Token>& tokens)
      : src_{tokens}, read_pos_{0} {
  }

  const Token& Read() {
    const Token& token = src_[read_pos_];
    if (read_pos_ < src_.size() - 1) {
      ++read_pos_;
    }
    return token;
  }

  const Token& Current() const {
    return src_[read_pos_];
  }

  bool Read(TokenType expected) {
    if (src_[read_pos_].type == expected) {
      if (read_pos_ < src_.size() - 1) {
        ++read_pos_;
      }
      return true;
    }
    return false;
  }

 private:
  const std::vector<Token>& src_;
  size_t read_pos_;
};

class ASTNode {
 public:
  virtual ~ASTNode() = default;
};

class Expression : public ASTNode {
};

class PrimaryExpression : public Expression {
};

class Literal : public PrimaryExpression {
};

class IntegerLiteral : public Literal {
 public:
  IntegerLiteral(int value) : value_{value} {
  }

  int GetValue() const {
    return value_;
  }

 private:
  int value_;
};

class Parser {
 public:
  Parser(TokenReader& reader) : reader_{reader} {
  }

  bool Parse() {
    auto ast = ParseIntegerLiteral();
    ast_root_ = ast;
    return ast_root_ != nullptr;
  }

  const std::shared_ptr<ASTNode> GetAST() const {
    return ast_root_;
  }

 private:
  TokenReader& reader_;
  std::shared_ptr<ASTNode> ast_root_;

  std::shared_ptr<IntegerLiteral> ParseIntegerLiteral() {
    auto token = reader_.Read();
    if (token.type == TokenType::kInteger) {
      return std::make_shared<IntegerLiteral>(token.int_value);
    }
    return {};
  }
};
