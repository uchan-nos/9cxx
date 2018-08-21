#include "parser.hpp"

#include <iostream>
#include "ast.hpp"

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

std::shared_ptr<ASTNode> Parse(TokenReader& reader) {
  Parser p{reader};
  if (!p.Parse()) {
    return {};
  }
  return p.GetAST();
}
