#pragma once

#include <vector>
#include <memory>

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
