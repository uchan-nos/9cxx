#include <cstdio>
#include <cstring>
#include <iostream>
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <boost/format.hpp>

#include "tokenizer.hpp"
#include "parser.hpp"
#include "ast.hpp"

#define MAX_SOURCE_LENGTH (1024*1024)

bool leading_underscore = true;

template <typename T>
boost::format& BoostFormat(boost::format& format_string, T head) {
  return format_string % head;
}

template <typename Head, typename... Tail>
boost::format& BoostFormat(boost::format& format_string, Head head, Tail... tail) {
  return BoostFormat(format_string % head, tail...);
}

class AssemblyLine {
 public:
  AssemblyLine(const std::string& line) : line_{line} {
  }

  template <typename... Args>
  AssemblyLine& Format(Args... args) {
    auto fmt = boost::format(line_);
    line_ = BoostFormat(fmt, args...).str();
    return *this;
  }

  const std::string& ToString() const {
    return line_;
  }

 private:
  std::string line_;
};

enum class IdType {
  kUnknown,
  kLocalVariable,
  kGlobal,
};

struct IdInfo {
  IdType type;
  size_t rbp_offset; // [rbp - rbp_offset]
};

const std::array<std::string, 6> kParamRegList{
  "rdi",
  "rsi",
  "rdx",
  "rcx",
  "r8",
  "r9",
};

std::string ExternName(const std::string& id_name) {
  if (leading_underscore) {
    return '_' + id_name;
  }
  return id_name;
}

class BaseVisitor : public Visitor {
 public:
  void Visit(TranslationUnit* unit, bool lvalue) {
    for (const auto& decl : unit->decls) {
      decl->Accept(this, lvalue);
    }
  }
  void Visit(CompoundStatement* stmt, bool lvalue) {}
  void Visit(ExpressionStatement* stmt, bool lvalue) {}
  void Visit(DeclarationStatement* stmt, bool lvalue) {}
  void Visit(AssignmentExpression* exp, bool lvalue) {}
  void Visit(EqualityExpression* exp, bool lvalue) {}
  void Visit(AdditiveExpression* exp, bool lvalue) {}
  void Visit(MultiplicativeExpression* exp, bool lvalue) {}
  void Visit(FunctionCallExpression* exp, bool lvalue) {}
  void Visit(IntegerLiteral* exp, bool lvalue) {}
  void Visit(Identifier* exp, bool lvalue) {}
  void Visit(SimpleDeclaration* decl, bool lvalue) {}
  void Visit(SimpleTypeSpecifier* spec, bool lvalue) {}
  void Visit(InitDeclarator* dtor, bool lvalue) {
    dtor->dtor->Accept(this, lvalue);
    if (dtor->init) dtor->init->Accept(this, lvalue);
  }
  void Visit(EqualInitializer* init, bool lvalue) {
    init->clause->Accept(this, lvalue);
  }
  void Visit(InitializerClause* clause, bool lvalue) {
    if (clause->assign) clause->assign->Accept(this, lvalue);
    if (clause->braced) clause->braced->Accept(this, lvalue);
  }
  void Visit(NoPtrDeclarator* dtor, bool lvalue) {
    dtor->id->Accept(this, lvalue);
  }
  void Visit(FunctionDeclarator* dtor, bool lvalue) {
    dtor->decl->Accept(this, lvalue);
    dtor->param->Accept(this, lvalue);
  }
  void Visit(ParameterDeclaration* decl, bool lvalue) {
    decl->spec->Accept(this, lvalue);
    decl->dtor->Accept(this, lvalue);
  }
  void Visit(ParametersAndQualifiers* pq, bool lvalue) {
    for (const auto& decl : pq->params) {
      decl->Accept(this, lvalue);
    }
  }
  void Visit(FunctionDefinition* defn, bool lvalue) {
    for (const auto& spec : defn->specs) {
      spec->Accept(this, lvalue);
    }
    defn->dtor->Accept(this, lvalue);
    defn->body->Accept(this, lvalue);
  }
};

class DeclSpecifierVisitor : public BaseVisitor {
 public:
  void Visit(SimpleTypeSpecifier* spec, bool lvalue) {
    BaseVisitor::Visit(spec, lvalue);
    simple_type_specifier_ = spec;
  }

  SimpleTypeSpecifier* SimpleTypeSpecifier() {
    return simple_type_specifier_;
  }

 private:
  struct SimpleTypeSpecifier* simple_type_specifier_ = nullptr;
};

class InitDeclaratorVisitor : public BaseVisitor {
 public:
  void Visit(InitializerClause* clause, bool lvalue) {
    BaseVisitor::Visit(clause, lvalue);
    initializer_clause_= clause;
  }

  void Visit(Identifier* id, bool lvalue) {
    BaseVisitor::Visit(id, lvalue);
    id_ = id;
  }

  void Visit(FunctionDeclarator* dtor, bool lvalue) {
    BaseVisitor::Visit(dtor, lvalue);
    function_declarator_ = dtor;
  }

  InitializerClause* InitializerClause() {
    return initializer_clause_;
  }

  Identifier* Identifier() {
    return id_;
  }

  FunctionDeclarator* FunctionDeclarator() {
    return function_declarator_;
  }

 private:
  struct InitializerClause* initializer_clause_ = nullptr;
  struct Identifier* id_ = nullptr;
  struct FunctionDeclarator* function_declarator_ = nullptr;
};

class CodeGenerateVisitor : public BaseVisitor {
 public:
  CodeGenerateVisitor(std::vector<AssemblyLine>& code)
      : code_{code}, ids_{}, last_rbp_offset_{0} {
  }

  void Visit(CompoundStatement* stmt, bool lvalue) {
    if (stmt->statements.empty()) {
      code_.push_back(AssemblyLine("  xor rax, rax"));
      return;
    }

    code_.push_back(AssemblyLine("  push rbp"));
    code_.push_back(AssemblyLine("  mov rbp, rsp"));
    code_.push_back(AssemblyLine("  sub rsp, %1%"));
    size_t rsp_line = code_.size() - 1;

    for (auto& n : stmt->statements) {
      n->Accept(this, lvalue);
    }

    size_t stack_size = (last_rbp_offset_ + 15) & ~static_cast<size_t>(15);
    if (stack_size == 0) {
      code_.erase(code_.begin() + rsp_line);
    } else {
      code_[rsp_line].Format(stack_size);
      code_.push_back(AssemblyLine("  mov rsp, rbp"));
    }
    code_.push_back(AssemblyLine("  pop rbp"));
  }

  void Visit(ExpressionStatement* stmt, bool lvalue) {
    stmt->exp->Accept(this, lvalue);
  }

  void Visit(DeclarationStatement* stmt, bool lvalue) {
    stmt->decl->Accept(this, lvalue);
  }

  void Visit(AssignmentExpression* exp, bool lvalue) {
    if (auto n = std::dynamic_pointer_cast<Identifier>(exp->lhs)) {
      const auto& id_name = n->value;
      if (ids_.find(id_name) == ids_.end()) {
        std::cerr << "Undeclared identifier: " << id_name << std::endl;
        return;
      }
    }

    exp->rhs->Accept(this, false);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this, true);
    code_.push_back(AssemblyLine("  pop rbx"));

    code_.push_back(AssemblyLine("  mov [rax], rbx"));
    if (!lvalue) {
      code_.push_back(AssemblyLine("  mov rax, rbx"));
    }
  }

  void Visit(EqualityExpression* exp, bool lvalue) {
    exp->rhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  pop rbx"));

    const char* op_mnemonic = "";
    if (exp->op == TokenType::kOpEqual) {
      op_mnemonic = "sete";
    } else if (exp->op == TokenType::kOpNotEqual) {
      op_mnemonic = "setne";
    }
    code_.push_back(AssemblyLine("  cmp eax, ebx"));
    code_.push_back(AssemblyLine("  %1% bl").Format(op_mnemonic));
    code_.push_back(AssemblyLine("  xor rax, rax"));
    code_.push_back(AssemblyLine("  mov al, bl"));
  }

  void Visit(AdditiveExpression* exp, bool lvalue) {
    exp->rhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  pop rbx"));

    const char* op_mnemonic = "";
    if (exp->op == TokenType::kOpPlus) {
      op_mnemonic = "add";
    } else if (exp->op == TokenType::kOpMinus) {
      op_mnemonic = "sub";
    }
    code_.push_back(AssemblyLine("  %1% eax, ebx").Format(op_mnemonic));
  }

  void Visit(MultiplicativeExpression* exp, bool lvalue) {
    exp->rhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  push rax"));
    exp->lhs->Accept(this, lvalue);
    code_.push_back(AssemblyLine("  pop rbx"));

    const char* op_mnemonic = "";
    if (exp->op == TokenType::kOpMult) {
      op_mnemonic = "mul";
    } else if (exp->op == TokenType::kOpDiv) {
      op_mnemonic = "div";
    }
    code_.push_back(AssemblyLine("  xor rdx, rdx"));
    code_.push_back(AssemblyLine("  %1% ebx").Format(op_mnemonic));
  }

  void Visit(FunctionCallExpression* exp, bool lvalue) {
    if (auto n = std::dynamic_pointer_cast<Identifier>(exp->name)) {
      const auto& id_name = n->value;
      if (ids_.find(id_name) == ids_.end()) {
        std::cerr << "Undeclared identifier: " << id_name << std::endl;
        return;
      }
    }

    for (size_t i = 0; i < exp->args.size(); ++i) {
      // reverse
      exp->args[exp->args.size() - i - 1]->Accept(this, false);
      code_.push_back(AssemblyLine("  push rax"));
    }
    for (size_t i = 0; i < exp->args.size(); ++i) {
      if (i == kParamRegList.size()) break;
      code_.push_back(AssemblyLine("  pop %1%").Format(kParamRegList[i]));
    }
    exp->name->Accept(this, true);
    code_.push_back(AssemblyLine("  call rax"));
  }

  void Visit(IntegerLiteral* exp, bool lvalue) {
    code_.push_back(AssemblyLine("  mov eax, %1%").Format(exp->value));
  }

  void Visit(Identifier* exp, bool lvalue) {
    const char* op_mnemonic = "";
    const auto& id_name = exp->value;

    if (lvalue) {
      op_mnemonic = "lea";
    } else {
      op_mnemonic = "mov";
    }

    if (ids_[id_name].type == IdType::kLocalVariable) {
      code_.push_back(AssemblyLine("  %1% rax, [rbp - %2%]").Format(
            op_mnemonic, ids_[id_name].rbp_offset));
    } else if (ids_[id_name].type == IdType::kGlobal) {
      code_.push_back(AssemblyLine("  mov rax, %1%").Format(ExternName(id_name)));
    } else {
      std::cerr << "Undefined symbol: " << id_name << std::endl;
    }
  }

  void Visit(SimpleDeclaration* decl, bool lvalue) {
    DeclSpecifierVisitor v;
    for (const auto& spec : decl->specs) {
      spec->Accept(&v, false);
    }

    for (const auto& init_decl : decl->dtors) {
      InitDeclaratorVisitor v2;
      init_decl->Accept(&v2, false);
      if (v2.FunctionDeclarator()) {
        const auto& id_name = v2.Identifier()->value;
        ids_[id_name].type = IdType::kGlobal;
        code_.push_back(AssemblyLine("  extern %1%").Format(ExternName(id_name)));
      } else {
        last_rbp_offset_ += 8;
        ids_[v2.Identifier()->value].type = IdType::kLocalVariable;
        ids_[v2.Identifier()->value].rbp_offset = last_rbp_offset_;
      }
    }
  }

  void Visit(SimpleTypeSpecifier* spec, bool lvalue) {
  }

  void Visit(InitDeclarator* dtor, bool lvalue) {
  }

  void Visit(EqualInitializer* init, bool lvalue) {
  }

  void Visit(InitializerClause* clause, bool lvalue) {
    if (clause->assign) {
      clause->assign->Accept(this, lvalue);
    } else {
      clause->braced->Accept(this, lvalue);
    }
  }

  void Visit(NoPtrDeclarator* dtor, bool lvalue) {
  }

  void Visit(FunctionDefinition* defn, bool lvalue) {
    DeclSpecifierVisitor v;
    for (const auto& spec : defn->specs) {
      spec->Accept(&v, false);
    }

    InitDeclaratorVisitor v2;
    defn->dtor->Accept(&v2, false);
    if (!v2.Identifier()) {
      std::cerr << "funcname must be specified" << std::endl;
      return;
    }

    const auto& id_name = v2.Identifier()->value;
    ids_[id_name].type = IdType::kGlobal;
    auto extern_name = ExternName(id_name);
    code_.push_back(AssemblyLine("global %1%").Format(extern_name));
    code_.push_back(AssemblyLine("%1%:").Format(extern_name));

    defn->body->Accept(this, false);

    code_.push_back(AssemblyLine("  ret"));
  }

 private:
  std::vector<AssemblyLine>& code_;
  std::map<std::string, IdInfo> ids_;
  size_t last_rbp_offset_;
};

class CodeGenerator {
 public:
  void Generate(std::shared_ptr<ASTNode> ast_root) {
    CodeGenerateVisitor visitor{code_};
    ast_root->Accept(&visitor, false);
  }

  const std::vector<AssemblyLine>& GetCode() const {
    return code_;
  }

 private:
  std::vector<AssemblyLine> code_;
};

int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) {
    if (strcmp("-fno-leading-underscore", argv[i]) == 0) {
      leading_underscore = false;
    }
  }

  char src[MAX_SOURCE_LENGTH];
  fread(src, MAX_SOURCE_LENGTH, 1, stdin);

  SourceReader src_reader{src};

  std::vector<Token> tokens;
  auto result = Tokenize(src_reader, tokens);
  if (!result.success) {
    fprintf(stderr, "Tokenize failed at token %lu\n", result.value);
    for (size_t i = 0; i < result.value; ++i) {
      fprintf(stderr, " %s", GetTokenName(tokens[i].type));
    }
    fprintf(stderr, "\n");
    return -1;
  }

  TokenReader token_reader{tokens};

  auto ast = Parse(token_reader);
  if (!ast) {
    fprintf(stderr, "Parse error\n");
    return -1;
  }

  CodeGenerator generator;
  generator.Generate(ast);
  for (auto& line : generator.GetCode()) {
    std::cout << line.ToString() << std::endl;
  }
  return 0;
}
