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
struct TranslationUnit;
struct CompoundStatement;
struct ExpressionStatement;
struct DeclarationStatement;
struct AssignmentExpression;
struct EqualityExpression;
struct AdditiveExpression;
struct MultiplicativeExpression;
struct FunctionCallExpression;
struct UnaryExpression;
struct IntegerLiteral;
struct Identifier;
struct Declaration;
struct BlockDeclaration;
struct SimpleDeclaration;
struct DeclSpecifier;
struct SimpleTypeSpecifier;
struct InitDeclarator;
struct Initializer;
struct EqualInitializer;
struct InitializerClause;
struct BracedInitList;
struct Declarator;
struct NoPtrDeclarator;
struct FunctionDeclarator;
struct ParameterDeclaration;
struct ParametersAndQualifiers;
struct FunctionDefinition;

class Visitor {
 public:
  virtual ~Visitor() = default;
  virtual void Visit(TranslationUnit* unit, bool lvalue) = 0;
  virtual void Visit(CompoundStatement* stmt, bool lvalue) = 0;
  virtual void Visit(ExpressionStatement* stmt, bool lvalue) = 0;
  virtual void Visit(DeclarationStatement* stmt, bool lvalue) = 0;
  virtual void Visit(AssignmentExpression* exp, bool lvalue) = 0;
  virtual void Visit(EqualityExpression* exp, bool lvalue) = 0;
  virtual void Visit(AdditiveExpression* exp, bool lvalue) = 0;
  virtual void Visit(MultiplicativeExpression* exp, bool lvalue) = 0;
  virtual void Visit(FunctionCallExpression* exp, bool lvalue) = 0;
  virtual void Visit(IntegerLiteral* exp, bool lvalue) = 0;
  virtual void Visit(Identifier* exp, bool lvalue) = 0;
  virtual void Visit(SimpleDeclaration* decl, bool lvalue) = 0;
  virtual void Visit(SimpleTypeSpecifier* spec, bool lvalue) = 0;
  virtual void Visit(InitDeclarator* dtor, bool lvalue) = 0;
  virtual void Visit(EqualInitializer* init, bool lvalue) = 0;
  virtual void Visit(InitializerClause* clause, bool lvalue) = 0;
  virtual void Visit(NoPtrDeclarator* dtor, bool lvalue) = 0;
  virtual void Visit(FunctionDeclarator* dtor, bool lvalue) = 0;
  virtual void Visit(ParameterDeclaration* decl, bool lvalue) = 0;
  virtual void Visit(ParametersAndQualifiers* pq, bool lvalue) = 0;
  virtual void Visit(FunctionDefinition* defn, bool lvalue) = 0;
};

struct ASTNode {
  virtual ~ASTNode() = default;
  virtual void Accept(Visitor* visitor, bool lvalue) = 0;
};

#define ACCEPT \
  void Accept(Visitor* visitor, bool lvalue) { visitor->Visit(this, lvalue); }

struct TranslationUnit : public ASTNode {
  std::vector<std::shared_ptr<Declaration>> decls;
  ACCEPT
};

struct Statement : public ASTNode {
};

struct Expression : public ASTNode {
};

struct CompoundStatement : public Statement {
  std::vector<std::shared_ptr<Statement>> statements;

  ACCEPT
};

struct ExpressionStatement : public Statement {
  std::shared_ptr<Expression> exp;

  ACCEPT
};

struct DeclarationStatement : public Statement {
  std::shared_ptr<BlockDeclaration> decl;

  ACCEPT
};

struct BinaryExpression : public Expression {
  std::shared_ptr<Expression> lhs;
  TokenType op;
  std::shared_ptr<Expression> rhs;
};

struct AssignmentExpression : public BinaryExpression {
  ACCEPT
};

struct EqualityExpression : public BinaryExpression {
  ACCEPT
};

struct AdditiveExpression : public BinaryExpression {
  ACCEPT
};

struct MultiplicativeExpression : public BinaryExpression {
  ACCEPT
};

struct FunctionCallExpression : public Expression {
  std::shared_ptr<Expression> name;
  std::vector<std::shared_ptr<InitializerClause>> args;

  ACCEPT
};

struct IntegerLiteral : public Expression {
  int value;

  ACCEPT
};

struct Identifier : public Expression {
  std::string value;

  ACCEPT
};

struct Declaration : public ASTNode {
};

struct BlockDeclaration : public Declaration {
};

struct SimpleDeclaration : public BlockDeclaration {
  std::vector<std::shared_ptr<DeclSpecifier>> specs;
  std::vector<std::shared_ptr<InitDeclarator>> dtors;;
  ACCEPT
};

struct DeclSpecifier : public ASTNode {
};

struct SimpleTypeSpecifier : public DeclSpecifier {
  std::string type;
  ACCEPT
};

struct InitDeclarator : public ASTNode {
  std::shared_ptr<Declarator> dtor;
  std::shared_ptr<Initializer> init;
  ACCEPT
};

struct Initializer : public ASTNode {
};

struct EqualInitializer : public Initializer {
  std::shared_ptr<InitializerClause> clause;
  ACCEPT
};

struct InitializerClause : public ASTNode {
  std::shared_ptr<Expression> assign;
  std::shared_ptr<BracedInitList> braced;
  ACCEPT
};

struct BracedInitList : public ASTNode {
  std::vector<std::shared_ptr<InitializerClause>> clauses;
};

struct Declarator : public ASTNode {
};

struct NoPtrDeclarator : public Declarator {
  std::shared_ptr<Identifier> id;
  ACCEPT
};

struct FunctionDeclarator : public Declarator {
  std::shared_ptr<NoPtrDeclarator> decl;
  std::shared_ptr<ParametersAndQualifiers> param;
  ACCEPT
};

struct ParameterDeclaration : public Declaration {
  std::shared_ptr<DeclSpecifier> spec;
  std::shared_ptr<Declarator> dtor;
  ACCEPT
};

struct ParametersAndQualifiers : public ASTNode {
  std::vector<std::shared_ptr<ParameterDeclaration>> params;
  bool omit; // ...
  ACCEPT
};

struct FunctionDefinition : public Declaration {
  std::vector<std::shared_ptr<DeclSpecifier>> specs;
  std::shared_ptr<Declarator> dtor;
  std::shared_ptr<Statement> body;
  ACCEPT
};

const std::set<std::string> kBasicTypes{
  "char",
  "int",
};

class Parser {
 public:
  Parser(TokenReader& reader) : reader_{reader} {
  }

  bool Parse() {
    auto n = std::make_shared<TranslationUnit>();
    std::cerr << "parsing translation unit (parsing declaration)" << std::endl;
    auto decl = ParseDeclaration();
    while (decl) {
      n->decls.push_back(decl);
      std::cerr << "parsing translation unit (parsing declaration)" << std::endl;
      decl = ParseDeclaration();
    }
    ast_root_ = n;
    return reader_.Read(TokenType::kEOF);
  }

  const std::shared_ptr<ASTNode> GetAST() const {
    return ast_root_;
  }

 private:
  TokenReader& reader_;
  std::shared_ptr<ASTNode> ast_root_;

  std::shared_ptr<Statement> ParseStatement() {
    std::cerr << "ParseStatement: begin. current token is " << GetTokenName(reader_.Current().type) << std::endl;
    if (reader_.Current().type == TokenType::kLBrace) {
      std::cerr << "parsing comp stmt" << std::endl;
      return ParseCompoundStatement();
    } else if (reader_.Current().type == TokenType::kKeyword) {
      std::cerr << "token is keyword --> parsing decl stmt" << std::endl;
      auto stmt = ParseDeclarationStatement();
      std::cerr << "  parsed decl stmt" << std::endl;
      return stmt;
    }
    std::cerr << "token is not keyword --> parsing exp stmt" << std::endl;
    return ParseExpressionStatement();
  }

  std::shared_ptr<Statement> ParseCompoundStatement() {
    std::cerr << "ParseCompoundStatemnt: begin. current token is " << GetTokenName(reader_.Current().type) << std::endl;
    if (!reader_.Read(TokenType::kLBrace)) {
      return {};
    }

    std::vector<std::shared_ptr<Statement>> statements;

    while (reader_.Current().type != TokenType::kRBrace) {
      std::cerr << "ParseCompoundStatement: parsing a statement" << std::endl;
      auto stmt = ParseStatement();
      if (!stmt) {
        std::cerr << "ParseCompoundStatement: A statement should be there."
                  << std::endl;
        return {};
      }
      statements.push_back(stmt);
    }

    if (!reader_.Read(TokenType::kRBrace)) {
      std::cerr << "ParseCompoundStatement: kRBrace is needed. actual "
                << GetTokenName(reader_.Current().type) << std::endl;
      return {};
    }

    auto n = std::make_shared<CompoundStatement>();
    n->statements = statements;
    return n;
  }

  std::shared_ptr<Statement> ParseDeclarationStatement() {
    auto decl = ParseBlockDeclaration();
    if (!decl) return {};

    auto n = std::make_shared<DeclarationStatement>();
    n->decl = decl;
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
    return ParseAssignmentExpression();
  }

  std::shared_ptr<Expression> ParseAssignmentExpression() {
    auto lhs = ParseEqualityExpression();

    TokenType op;
    if (reader_.Read(TokenType::kOpAssign)) {
      op = TokenType::kOpAssign;
    } else {
      return lhs;
    }

    auto rhs = ParseAssignmentExpression();
    if (!rhs) {
      return {};
    }

    auto n = std::make_shared<AssignmentExpression>();
    n->lhs = lhs;
    n->op = op;
    n->rhs = rhs;
    return n;
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
    auto lhs = ParsePostfixExpression();

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

  std::shared_ptr<Expression> ParsePostfixExpression() {
    auto main = ParsePrimaryExpression();

    if (reader_.Read(TokenType::kLParen)) {
      auto n = std::make_shared<FunctionCallExpression>();
      n->name = main;
      auto arg = ParseInitializerClause();
      if (arg) n->args.push_back(arg);
      while (reader_.Read(TokenType::kComma)) {
        arg = ParseInitializerClause();
        if (!arg) return {};
        n->args.push_back(arg);
      }
      if (reader_.Read(TokenType::kRParen)) {
        return n;
      }
      return {};
    } else {
      return main;
    }

    return {};
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
      auto token = reader_.Read();
      auto n = std::make_shared<Identifier>();
      n->value = token.string_value;
      return n;
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

  std::shared_ptr<Declaration> ParseDeclaration() {
    std::cerr << "ParseDeclaration" << std::endl;
    auto specs = ParseDeclSpecifierSeq();
    if (specs.empty()) return {};

    auto dtor = ParseDeclarator();
    if (!dtor) return {};

    if (reader_.Current().type == TokenType::kLBrace) {
      auto body = ParseCompoundStatement();
      auto n = std::make_shared<FunctionDefinition>();
      n->specs = specs;
      n->dtor = dtor;
      n->body = body;
      return n;
    }

    return ParseBlockDeclaration(specs, dtor);
  }

  std::shared_ptr<BlockDeclaration> ParseBlockDeclaration(
      const std::vector<std::shared_ptr<DeclSpecifier>>& specs,
      const std::shared_ptr<Declarator>& dtor) {
    return ParseSimpleDeclaration(specs, dtor);
  }

  std::shared_ptr<BlockDeclaration> ParseBlockDeclaration() {
    std::cerr << "ParseBlockDeclaration" << std::endl;
    auto specs = ParseDeclSpecifierSeq();
    if (specs.empty()) return {};
    auto dtor = ParseDeclarator();
    return ParseBlockDeclaration(specs, dtor);
  }

  std::shared_ptr<SimpleDeclaration> ParseSimpleDeclaration(
      const std::vector<std::shared_ptr<DeclSpecifier>>& specs,
      const std::shared_ptr<Declarator>& dtor) {
    auto n = std::make_shared<SimpleDeclaration>();
    n->specs = specs;

    auto init_dtor = ParseInitDeclarator(dtor);
    if (init_dtor) n->dtors.push_back(init_dtor);
    while (reader_.Read(TokenType::kComma)) {
      init_dtor = ParseInitDeclarator();
      if (!init_dtor) return {};
      n->dtors.push_back(init_dtor);
    }
    std::cerr << "ParseSimpleDeclaration: size of dtors = " << n->dtors.size() << std::endl;

    if (!reader_.Read(TokenType::kSemicolon)) {
      return {};
    }
    return n;
  }

  std::vector<std::shared_ptr<DeclSpecifier>> ParseDeclSpecifierSeq() {
    std::vector<std::shared_ptr<DeclSpecifier>> specs;
    auto spec = ParseDeclSpecifier();
    while (spec) {
      specs.push_back(spec);
      spec = ParseDeclSpecifier();
    }
    return specs;
  }

  std::shared_ptr<DeclSpecifier> ParseDeclSpecifier() {
    return ParseSimpleTypeSpecifier();
  }

  std::shared_ptr<SimpleTypeSpecifier> ParseSimpleTypeSpecifier() {
    if (reader_.Current().type != TokenType::kKeyword) {
      return {};
    }
    auto token = reader_.Read();
    if (kBasicTypes.find(token.string_value) == kBasicTypes.end()) {
      return {};
    }
    auto n = std::make_shared<SimpleTypeSpecifier>();
    n->type = token.string_value;
    return n;
  }

  std::shared_ptr<InitDeclarator> ParseInitDeclarator(
      const std::shared_ptr<Declarator>& dtor) {
    if (!dtor) {
      return {};
    }
    auto n = std::make_shared<InitDeclarator>();
    n->dtor = dtor;
    n->init = ParseInitializer();;
    return n;
  }

  std::shared_ptr<InitDeclarator> ParseInitDeclarator() {
    std::cerr << "ParseInitDeclarator" << std::endl;
    return ParseInitDeclarator(ParseDeclarator());
  }

  std::shared_ptr<Initializer> ParseInitializer() {
    return ParseEqualInitializer();
  }

  std::shared_ptr<EqualInitializer> ParseEqualInitializer() {
    auto clause = ParseInitializerClause();
    if (!clause) {
      return {};
    }
    auto n = std::make_shared<EqualInitializer>();
    n->clause = clause;
    return n;
  }

  std::shared_ptr<InitializerClause> ParseInitializerClause() {
    auto assign = ParseAssignmentExpression();
    std::shared_ptr<BracedInitList> braced;
    if (!assign) {
      //braced = ParseBracedInitList;
    }
    if (!assign && !braced) {
      return {};
    }
    auto n = std::make_shared<InitializerClause>();
    n->assign = assign;
    n->braced = braced;
    return n;
  }

  std::shared_ptr<Declarator> ParseDeclarator() {
    auto decl = ParseNoPtrDeclarator();
    if (auto param = ParseParametersAndQualifiers()) {
      auto n = std::make_shared<FunctionDeclarator>();
      std::cerr << "function dtor" << std::endl;
      n->decl = decl;
      n->param = param;
      return n;
    }
    return decl;
  }

  std::shared_ptr<NoPtrDeclarator> ParseNoPtrDeclarator() {
    if (reader_.Current().type != TokenType::kId) {
      return {};
    }
    auto id = reader_.Read().string_value;
    auto n = std::make_shared<NoPtrDeclarator>();
    n->id = std::make_shared<Identifier>();
    n->id->value = id;
    return n;
  }

  std::shared_ptr<ParametersAndQualifiers> ParseParametersAndQualifiers() {
    if (!reader_.Read(TokenType::kLParen)) {
      return {};
    }
    auto n = std::make_shared<ParametersAndQualifiers>();
    auto decl = ParseParameterDeclaration();
    if (decl) n->params.push_back(decl);
    while (reader_.Read(TokenType::kComma)) {
      decl = ParseParameterDeclaration();
      if (!decl) return {};
      n->params.push_back(decl);
    }
    if (!reader_.Read(TokenType::kRParen)) {
      return {};
    }
    n->omit = false;
    return n;
  }

  std::shared_ptr<ParameterDeclaration> ParseParameterDeclaration() {
    auto spec = ParseDeclSpecifier();
    if (!spec) {
      return {};
    }
    auto dtor = ParseDeclarator();
    if (!dtor) {
      return {};
    }
    auto n = std::make_shared<ParameterDeclaration>();
    n->spec = spec;
    n->dtor = dtor;
    return n;
  }
};
