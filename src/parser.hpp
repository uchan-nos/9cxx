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

struct ASTNode;
struct CompoundStatement;
struct ExpressionStatement;
struct EqualityExpression;
struct AdditiveExpression;
struct MultiplicativeExpression;
struct IntegerLiteral;

class Visitor {
 public:
  virtual ~Visitor() = default;
  virtual void Visit(ASTNode* n) = 0;
  virtual void Visit(CompoundStatement* stmt) = 0;
  virtual void Visit(ExpressionStatement* stmt) = 0;
  virtual void Visit(EqualityExpression* exp) = 0;
  virtual void Visit(AdditiveExpression* exp) = 0;
  virtual void Visit(MultiplicativeExpression* exp) = 0;
  virtual void Visit(IntegerLiteral* exp) = 0;
};

struct ASTNode {
  virtual ~ASTNode() = default;
  virtual void Accept(Visitor* visitor) = 0;
};

void ASTNode::Accept(Visitor* visitor) {
  visitor->Visit(this);
}

struct Statement : public ASTNode {
};

struct Expression : public ASTNode {
};

struct CompoundStatement : public Statement {
  std::vector<std::shared_ptr<Statement>> statements;

  void Accept(Visitor* visitor) {
    visitor->Visit(this);
  }
};

struct ExpressionStatement : public Statement {
  std::shared_ptr<Expression> exp;

  void Accept(Visitor* visitor) {
    visitor->Visit(this);
  }
};

struct BinaryExpression : public Expression {
  std::shared_ptr<Expression> lhs;
  TokenType op;
  std::shared_ptr<Expression> rhs;

  void Accept(Visitor* visitor) {
    visitor->Visit(this);
  }
};

struct EqualityExpression : public BinaryExpression {
  void Accept(Visitor* visitor) {
    visitor->Visit(this);
  }
};

struct AdditiveExpression : public BinaryExpression {
  void Accept(Visitor* visitor) {
    visitor->Visit(this);
  }
};

struct MultiplicativeExpression : public BinaryExpression {
  void Accept(Visitor* visitor) {
    visitor->Visit(this);
  }
};

struct IntegerLiteral : public Expression {
  int value;

  void Accept(Visitor* visitor) {
    visitor->Visit(this);
  }
};


class Parser {
 public:
  Parser(TokenReader& reader) : reader_{reader} {
  }

  bool Parse() {
    auto ast = ParseStatement();
    ast_root_ = ast;
    return reader_.Read(TokenType::kEOF);
  }

  const std::shared_ptr<ASTNode> GetAST() const {
    return ast_root_;
  }

 private:
  TokenReader& reader_;
  std::shared_ptr<ASTNode> ast_root_;

  std::shared_ptr<Statement> ParseStatement() {
    if (reader_.Current().type == TokenType::kLBrace) {
      return ParseCompoundStatement();
    }
    return ParseExpressionStatement();
  }

  std::shared_ptr<Statement> ParseCompoundStatement() {
    if (!reader_.Read(TokenType::kLBrace)) {
      return {};
    }

    std::vector<std::shared_ptr<Statement>> statements;
    auto stmt = ParseStatement();
    while (stmt) {
      statements.push_back(stmt);
      stmt = ParseStatement();
    }

    if (!reader_.Read(TokenType::kRBrace)) {
      return {};
    }

    auto n = std::make_shared<CompoundStatement>();
    n->statements = statements;
    return n;
  }

  std::shared_ptr<Statement> ParseExpressionStatement() {
    auto exp = ParseExpression();
    if (!exp || !reader_.Read(TokenType::kSemicolon)) {
      return {};
    }

    auto n = std::make_shared<ExpressionStatement>();
    n->exp = exp;
    return n;
  }

  std::shared_ptr<Expression> ParseExpression() {
    return ParseEqualityExpression();
  }

  std::shared_ptr<Expression> ParseEqualityExpression() {
    auto lhs = ParseAdditiveExpression();

    TokenType op;
    if (reader_.Read(TokenType::kOpEqual)) {
      op = TokenType::kOpEqual;
    } else if (reader_.Read(TokenType::kOpNotEqual)) {
      op = TokenType::kOpNotEqual;
    } else {
      return lhs;
    }

    auto rhs = ParseEqualityExpression();
    if (!rhs) {
      return {};
    }

    auto n = std::make_shared<EqualityExpression>();
    n->lhs = lhs;
    n->op = op;
    n->rhs = rhs;
    return n;
  }

  std::shared_ptr<Expression> ParseAdditiveExpression() {
    auto lhs = ParseMultiplicativeExpression();

    TokenType op;
    if (reader_.Read(TokenType::kOpPlus)) {
      op = TokenType::kOpPlus;
    } else if (reader_.Read(TokenType::kOpMinus)) {
      op = TokenType::kOpMinus;
    } else {
      return lhs;
    }

    auto rhs = ParseAdditiveExpression();
    if (!rhs) {
      return {};
    }

    auto n = std::make_shared<AdditiveExpression>();
    n->lhs = lhs;
    n->op = op;
    n->rhs = rhs;
    return n;
  }

  std::shared_ptr<Expression> ParseMultiplicativeExpression() {
    auto lhs = ParsePrimaryExpression();

    TokenType op;
    if (reader_.Read(TokenType::kOpMult)) {
      op = TokenType::kOpMult;
    } else if (reader_.Read(TokenType::kOpDiv)) {
      op = TokenType::kOpDiv;
    } else {
      return lhs;
    }

    auto rhs = ParseMultiplicativeExpression();
    if (!rhs) {
      return {};
    }

    auto n = std::make_shared<MultiplicativeExpression>();
    n->lhs = lhs;
    n->op = op;
    n->rhs = rhs;
    return n;
  }

  std::shared_ptr<Expression> ParsePrimaryExpression() {
    if (reader_.Read(TokenType::kLParen)) {
      auto exp = ParseExpression();
      auto token = reader_.Read();
      if (token.type == TokenType::kRParen) {
        return exp;
      }
      std::cerr << "ParsePrimaryExpression: kRParen expected: "
                << GetTokenName(token.type);
      return {};
    } else if (reader_.Current().type == TokenType::kId) {
      reader_.Read();
      return {};
    }
    return ParseLiteral();
  }

  std::shared_ptr<Expression> ParseLiteral() {
    if (reader_.Current().type == TokenType::kInteger) {
      return ParseIntegerLiteral();
    }
    return {};
  }

  std::shared_ptr<Expression> ParseIntegerLiteral() {
    auto token = reader_.Read();
    auto n = std::make_shared<IntegerLiteral>();
    n->value = token.int_value;
    return n;
  }
};
