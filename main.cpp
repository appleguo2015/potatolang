#include <cctype>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace potatolang {

enum class TokenType {
  LeftParen,
  RightParen,
  LeftBrace,
  RightBrace,
  Semicolon,
  Comma,

  Plus,
  Minus,
  Star,
  Slash,
  Bang,
  Equal,
  Less,
  Greater,

  BangEqual,
  EqualEqual,
  LessEqual,
  GreaterEqual,

  Identifier,
  Number,
  String,

  Let,
  Print,
  If,
  Else,
  While,
  Fun,
  Return,
  Import,
  True,
  False,
  Nil,
  And,
  Or,

  Eof,
  Invalid,
};

struct SourceLocation {
  int line = 1;
  int column = 1;
};

struct Token {
  TokenType type = TokenType::Invalid;
  std::string lexeme;
  SourceLocation loc;
};

static std::string TokenTypeName(TokenType t) {
  switch (t) {
    case TokenType::LeftParen: return "LeftParen";
    case TokenType::RightParen: return "RightParen";
    case TokenType::LeftBrace: return "LeftBrace";
    case TokenType::RightBrace: return "RightBrace";
    case TokenType::Semicolon: return "Semicolon";
    case TokenType::Comma: return "Comma";
    case TokenType::Plus: return "Plus";
    case TokenType::Minus: return "Minus";
    case TokenType::Star: return "Star";
    case TokenType::Slash: return "Slash";
    case TokenType::Bang: return "Bang";
    case TokenType::Equal: return "Equal";
    case TokenType::Less: return "Less";
    case TokenType::Greater: return "Greater";
    case TokenType::BangEqual: return "BangEqual";
    case TokenType::EqualEqual: return "EqualEqual";
    case TokenType::LessEqual: return "LessEqual";
    case TokenType::GreaterEqual: return "GreaterEqual";
    case TokenType::Identifier: return "Identifier";
    case TokenType::Number: return "Number";
    case TokenType::String: return "String";
    case TokenType::Let: return "Let";
    case TokenType::Print: return "Print";
    case TokenType::If: return "If";
    case TokenType::Else: return "Else";
    case TokenType::While: return "While";
    case TokenType::Fun: return "Fun";
    case TokenType::Return: return "Return";
    case TokenType::Import: return "Import";
    case TokenType::True: return "True";
    case TokenType::False: return "False";
    case TokenType::Nil: return "Nil";
    case TokenType::And: return "And";
    case TokenType::Or: return "Or";
    case TokenType::Eof: return "Eof";
    case TokenType::Invalid: return "Invalid";
  }
  return "Invalid";
}

class Lexer {
 public:
  explicit Lexer(std::string source) : source_(std::move(source)) {}

  // Scans all tokens from the source string.
  std::vector<Token> LexAll() {
    std::vector<Token> out;
    while (true) {
      Token t = NextToken();
      out.push_back(t);
      if (t.type == TokenType::Eof) break;
    }
    return out;
  }

 private:
  // Scans the next token.
  Token NextToken() {
    SkipWhitespaceAndComments();
    SourceLocation start = loc_;
    if (IsAtEnd()) return Make(TokenType::Eof, "", start);

    char c = Advance();
    switch (c) {
      case '(': return Make(TokenType::LeftParen, "(", start);
      case ')': return Make(TokenType::RightParen, ")", start);
      case '{': return Make(TokenType::LeftBrace, "{", start);
      case '}': return Make(TokenType::RightBrace, "}", start);
      case ';': return Make(TokenType::Semicolon, ";", start);
      case ',': return Make(TokenType::Comma, ",", start);
      case '+': return Make(TokenType::Plus, "+", start);
      case '-': return Make(TokenType::Minus, "-", start);
      case '*': return Make(TokenType::Star, "*", start);
      case '/': return Make(TokenType::Slash, "/", start);
      case '!': return Match('=') ? Make(TokenType::BangEqual, "!=", start) : Make(TokenType::Bang, "!", start);
      case '=': return Match('=') ? Make(TokenType::EqualEqual, "==", start) : Make(TokenType::Equal, "=", start);
      case '<': return Match('=') ? Make(TokenType::LessEqual, "<=", start) : Make(TokenType::Less, "<", start);
      case '>': return Match('=') ? Make(TokenType::GreaterEqual, ">=", start) : Make(TokenType::Greater, ">", start);
      case '"': return LexString(start);
      default: break;
    }

    if (std::isdigit(static_cast<unsigned char>(c))) return LexNumber(start, c);
    if (IsIdentStart(c)) return LexIdentifierOrKeyword(start, c);

    return Make(TokenType::Invalid, std::string(1, c), start);
  }

  // Scans a string literal.
  Token LexString(const SourceLocation& start) {
    std::string value;
    while (!IsAtEnd() && Peek() != '"') {
      char c = Advance();
      if (c == '\n') return Make(TokenType::Invalid, "Unterminated string", start);
      if (c == '\\') {
        if (IsAtEnd()) return Make(TokenType::Invalid, "Unterminated string", start);
        char e = Advance();
        switch (e) {
          case 'n': value.push_back('\n'); break;
          case 't': value.push_back('\t'); break;
          case '"': value.push_back('"'); break;
          case '\\': value.push_back('\\'); break;
          default: value.push_back(e); break;
        }
      } else {
        value.push_back(c);
      }
    }
    if (IsAtEnd()) return Make(TokenType::Invalid, "Unterminated string", start);
    Advance();
    return Make(TokenType::String, value, start);
  }

  // Scans a number literal.
  Token LexNumber(const SourceLocation& start, char first) {
    std::string s(1, first);
    while (!IsAtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) s.push_back(Advance());
    if (!IsAtEnd() && Peek() == '.' && std::isdigit(static_cast<unsigned char>(PeekNext()))) {
      s.push_back(Advance());
      while (!IsAtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) s.push_back(Advance());
    }
    return Make(TokenType::Number, s, start);
  }

  // Scans an identifier or a keyword.
  Token LexIdentifierOrKeyword(const SourceLocation& start, char first) {
    std::string s(1, first);
    while (!IsAtEnd() && IsIdentContinue(Peek())) s.push_back(Advance());
    if (s == "let") return Make(TokenType::Let, s, start);
    if (s == "print") return Make(TokenType::Print, s, start);
    if (s == "if") return Make(TokenType::If, s, start);
    if (s == "else") return Make(TokenType::Else, s, start);
    if (s == "while") return Make(TokenType::While, s, start);
    if (s == "fun") return Make(TokenType::Fun, s, start);
    if (s == "return") return Make(TokenType::Return, s, start);
    if (s == "import") return Make(TokenType::Import, s, start);
    if (s == "true") return Make(TokenType::True, s, start);
    if (s == "false") return Make(TokenType::False, s, start);
    if (s == "nil") return Make(TokenType::Nil, s, start);
    if (s == "and") return Make(TokenType::And, s, start);
    if (s == "or") return Make(TokenType::Or, s, start);
    return Make(TokenType::Identifier, s, start);
  }

  // Skips whitespace and comments.
  void SkipWhitespaceAndComments() {
    while (!IsAtEnd()) {
      char c = Peek();
      if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
        Advance();
        continue;
      }
      if (c == '/' && PeekNext() == '/') {
        while (!IsAtEnd() && Peek() != '\n') Advance();
        continue;
      }
      break;
    }
  }

  bool IsAtEnd() const { return index_ >= source_.size(); }

  char Peek() const { return IsAtEnd() ? '\0' : source_[index_]; }

  char PeekNext() const { return (index_ + 1 >= source_.size()) ? '\0' : source_[index_ + 1]; }

  char Advance() {
    char c = source_[index_++];
    if (c == '\n') {
      loc_.line += 1;
      loc_.column = 1;
    } else {
      loc_.column += 1;
    }
    return c;
  }

  bool Match(char expected) {
    if (IsAtEnd()) return false;
    if (source_[index_] != expected) return false;
    Advance();
    return true;
  }

  static bool IsIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
  static bool IsIdentContinue(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

  Token Make(TokenType type, std::string lexeme, const SourceLocation& start) {
    Token t;
    t.type = type;
    t.lexeme = std::move(lexeme);
    t.loc = start;
    return t;
  }

  std::string source_;
  std::size_t index_ = 0;
  SourceLocation loc_;
};

class ParseError : public std::runtime_error {
 public:
  ParseError(SourceLocation loc, std::string message)
      : std::runtime_error(BuildMessage(loc, message)), loc_(loc), message_(std::move(message)) {}

  SourceLocation loc() const { return loc_; }
  const std::string& message() const { return message_; }

 private:
  static std::string BuildMessage(const SourceLocation& loc, const std::string& message) {
    std::ostringstream oss;
    oss << "Parse error at " << loc.line << ":" << loc.column << ": " << message;
    return oss.str();
  }

  SourceLocation loc_;
  std::string message_;
};

struct Expr {
  virtual ~Expr() = default;
  virtual void Print(std::ostream& out) const = 0;
};

struct Stmt {
  virtual ~Stmt() = default;
  virtual void Print(std::ostream& out) const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct LiteralExpr : Expr {
  enum class Kind { Number, String, Bool, Nil };
  Kind kind;
  std::string value;

  LiteralExpr(Kind k, std::string v) : kind(k), value(std::move(v)) {}

  void Print(std::ostream& out) const override {
    switch (kind) {
      case Kind::Number: out << value; break;
      case Kind::String: out << '"' << Escape(value) << '"'; break;
      case Kind::Bool: out << value; break;
      case Kind::Nil: out << "nil"; break;
    }
  }

  static std::string Escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
      switch (c) {
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        default: out.push_back(c); break;
      }
    }
    return out;
  }
};

struct VariableExpr : Expr {
  Token name;
  explicit VariableExpr(Token n) : name(std::move(n)) {}
  void Print(std::ostream& out) const override { out << name.lexeme; }
};

struct GroupingExpr : Expr {
  ExprPtr expr;
  explicit GroupingExpr(ExprPtr e) : expr(std::move(e)) {}
  void Print(std::ostream& out) const override {
    out << "(group ";
    expr->Print(out);
    out << ")";
  }
};

struct UnaryExpr : Expr {
  Token op;
  ExprPtr right;
  UnaryExpr(Token o, ExprPtr r) : op(std::move(o)), right(std::move(r)) {}
  void Print(std::ostream& out) const override {
    out << "(" << op.lexeme << " ";
    right->Print(out);
    out << ")";
  }
};

struct BinaryExpr : Expr {
  ExprPtr left;
  Token op;
  ExprPtr right;
  BinaryExpr(ExprPtr l, Token o, ExprPtr r) : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
  void Print(std::ostream& out) const override {
    out << "(" << op.lexeme << " ";
    left->Print(out);
    out << " ";
    right->Print(out);
    out << ")";
  }
};

struct LogicalExpr : Expr {
  ExprPtr left;
  Token op;
  ExprPtr right;
  LogicalExpr(ExprPtr l, Token o, ExprPtr r) : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
  void Print(std::ostream& out) const override {
    out << "(" << op.lexeme << " ";
    left->Print(out);
    out << " ";
    right->Print(out);
    out << ")";
  }
};

struct CallExpr : Expr {
  ExprPtr callee;
  Token paren;
  std::vector<ExprPtr> args;
  CallExpr(ExprPtr c, Token p, std::vector<ExprPtr> a) : callee(std::move(c)), paren(std::move(p)), args(std::move(a)) {}
  void Print(std::ostream& out) const override {
    out << "(call ";
    callee->Print(out);
    for (const auto& a : args) {
      out << " ";
      a->Print(out);
    }
    out << ")";
  }
};

struct LetStmt : Stmt {
  Token name;
  ExprPtr init;
  LetStmt(Token n, ExprPtr i) : name(std::move(n)), init(std::move(i)) {}
  void Print(std::ostream& out) const override {
    out << "(let " << name.lexeme << " ";
    init->Print(out);
    out << ")";
  }
};

struct AssignStmt : Stmt {
  Token name;
  ExprPtr value;
  AssignStmt(Token n, ExprPtr v) : name(std::move(n)), value(std::move(v)) {}
  void Print(std::ostream& out) const override {
    out << "(assign " << name.lexeme << " ";
    value->Print(out);
    out << ")";
  }
};

struct PrintStmt : Stmt {
  ExprPtr expr;
  explicit PrintStmt(ExprPtr e) : expr(std::move(e)) {}
  void Print(std::ostream& out) const override {
    out << "(print ";
    expr->Print(out);
    out << ")";
  }
};

struct ExprStmt : Stmt {
  ExprPtr expr;
  explicit ExprStmt(ExprPtr e) : expr(std::move(e)) {}
  void Print(std::ostream& out) const override {
    out << "(expr ";
    expr->Print(out);
    out << ")";
  }
};

struct ImportStmt : Stmt {
  Token module;
  explicit ImportStmt(Token m) : module(std::move(m)) {}
  void Print(std::ostream& out) const override {
    out << "(import ";
    if (module.type == TokenType::String) {
      out << '"' << LiteralExpr::Escape(module.lexeme) << '"';
    } else {
      out << module.lexeme;
    }
    out << ")";
  }
};

struct BlockStmt : Stmt {
  std::vector<StmtPtr> statements;
  explicit BlockStmt(std::vector<StmtPtr> s) : statements(std::move(s)) {}
  void Print(std::ostream& out) const override {
    out << "(block";
    for (const auto& st : statements) {
      out << " ";
      st->Print(out);
    }
    out << ")";
  }
};

struct IfStmt : Stmt {
  ExprPtr condition;
  StmtPtr thenBranch;
  std::optional<StmtPtr> elseBranch;
  IfStmt(ExprPtr c, StmtPtr t, std::optional<StmtPtr> e)
      : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
  void Print(std::ostream& out) const override {
    out << "(if ";
    condition->Print(out);
    out << " ";
    thenBranch->Print(out);
    if (elseBranch.has_value()) {
      out << " ";
      (*elseBranch)->Print(out);
    }
    out << ")";
  }
};

struct WhileStmt : Stmt {
  ExprPtr condition;
  StmtPtr body;
  WhileStmt(ExprPtr c, StmtPtr b) : condition(std::move(c)), body(std::move(b)) {}
  void Print(std::ostream& out) const override {
    out << "(while ";
    condition->Print(out);
    out << " ";
    body->Print(out);
    out << ")";
  }
};

struct FunctionStmt : Stmt {
  Token name;
  std::vector<Token> params;
  std::vector<StmtPtr> body;
  FunctionStmt(Token n, std::vector<Token> p, std::vector<StmtPtr> b)
      : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
  void Print(std::ostream& out) const override {
    out << "(fun " << name.lexeme << " (params";
    for (const auto& p : params) out << " " << p.lexeme;
    out << ") (block";
    for (const auto& st : body) {
      out << " ";
      st->Print(out);
    }
    out << "))";
  }
};

struct ReturnStmt : Stmt {
  Token keyword;
  std::optional<ExprPtr> value;
  ReturnStmt(Token k, std::optional<ExprPtr> v) : keyword(std::move(k)), value(std::move(v)) {}
  void Print(std::ostream& out) const override {
    out << "(return";
    if (value.has_value()) {
      out << " ";
      (*value)->Print(out);
    }
    out << ")";
  }
};

class Parser {
 public:
  explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

  // Parses the full program into a list of statements.
  std::vector<StmtPtr> ParseProgram() {
    std::vector<StmtPtr> stmts;
    while (!Check(TokenType::Eof)) {
      stmts.push_back(ParseDeclaration());
    }
    return stmts;
  }

 private:
  // Parses a declaration (function, variable, or statement).
  StmtPtr ParseDeclaration() {
    if (Match(TokenType::Import)) return ParseImportStmt();
    if (Match(TokenType::Fun)) return ParseFunDecl();
    if (Match(TokenType::Let)) return ParseLetStmt();
    return ParseStmt();
  }

  // Parses an import statement.
  StmtPtr ParseImportStmt() {
    Token module = Match(TokenType::String) ? Previous() : Consume(TokenType::Identifier, "Expected module name after 'import'");
    Consume(TokenType::Semicolon, "Expected ';' after import statement");
    return std::make_unique<ImportStmt>(std::move(module));
  }

  StmtPtr ParseLetStmt() {
    Token name = Consume(TokenType::Identifier, "Expected identifier after 'let'");
    Consume(TokenType::Equal, "Expected '=' after variable name");
    ExprPtr init = ParseExpr();
    Consume(TokenType::Semicolon, "Expected ';' after let statement");
    return std::make_unique<LetStmt>(std::move(name), std::move(init));
  }

  StmtPtr ParsePrintStmt() {
    ExprPtr e = ParseExpr();
    Consume(TokenType::Semicolon, "Expected ';' after print statement");
    return std::make_unique<PrintStmt>(std::move(e));
  }

  // Parses a generic statement (expression, block, if, while, return, assign).
  StmtPtr ParseStmt() {
    if (Match(TokenType::LeftBrace)) return ParseBlockStmt();
    if (Match(TokenType::If)) return ParseIfStmt();
    if (Match(TokenType::While)) return ParseWhileStmt();
    if (Match(TokenType::Return)) return ParseReturnStmt();
    if (Check(TokenType::Identifier) && CheckNext(TokenType::Equal)) return ParseAssignStmt();
    if (Match(TokenType::Print)) return ParsePrintStmt();
    return ParseExprStmt();
  }

  // Parses an expression statement.
  StmtPtr ParseExprStmt() {
    auto expr = ParseExpr();
    Consume(TokenType::Semicolon, "Expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr));
  }

  StmtPtr ParseAssignStmt() {
    Token name = Consume(TokenType::Identifier, "Expected identifier");
    Consume(TokenType::Equal, "Expected '=' in assignment");
    ExprPtr value = ParseExpr();
    Consume(TokenType::Semicolon, "Expected ';' after assignment");
    return std::make_unique<AssignStmt>(std::move(name), std::move(value));
  }

  StmtPtr ParseBlockStmt() {
    std::vector<StmtPtr> statements;
    while (!Check(TokenType::RightBrace) && !Check(TokenType::Eof)) {
      statements.push_back(ParseDeclaration());
    }
    Consume(TokenType::RightBrace, "Expected '}' after block");
    return std::make_unique<BlockStmt>(std::move(statements));
  }

  // Parses an if statement.
  StmtPtr ParseIfStmt() {
    Consume(TokenType::LeftParen, "Expected '(' after 'if'");
    auto condition = ParseExpr();
    Consume(TokenType::RightParen, "Expected ')' after if condition");
    auto thenBranch = ParseStmt();
    std::optional<StmtPtr> elseBranch;
    if (Match(TokenType::Else)) {
      elseBranch = ParseStmt();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
  }

  // Parses a while loop.
  StmtPtr ParseWhileStmt() {
    Consume(TokenType::LeftParen, "Expected '(' after 'while'");
    auto condition = ParseExpr();
    Consume(TokenType::RightParen, "Expected ')' after while condition");
    auto body = ParseStmt();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
  }

  StmtPtr ParseFunDecl() {
    Token name = Consume(TokenType::Identifier, "Expected function name after 'fun'");
    Consume(TokenType::LeftParen, "Expected '(' after function name");
    std::vector<Token> params;
    if (!Check(TokenType::RightParen)) {
      do {
        params.push_back(Consume(TokenType::Identifier, "Expected parameter name"));
      } while (Match(TokenType::Comma));
    }
    Consume(TokenType::RightParen, "Expected ')' after parameters");
    Consume(TokenType::LeftBrace, "Expected '{' before function body");
    std::vector<StmtPtr> body;
    while (!Check(TokenType::RightBrace) && !Check(TokenType::Eof)) {
      body.push_back(ParseDeclaration());
    }
    Consume(TokenType::RightBrace, "Expected '}' after function body");
    return std::make_unique<FunctionStmt>(std::move(name), std::move(params), std::move(body));
  }

  StmtPtr ParseReturnStmt() {
    Token keyword = Previous();
    if (Check(TokenType::Semicolon)) {
      Advance();
      return std::make_unique<ReturnStmt>(std::move(keyword), std::nullopt);
    }
    ExprPtr value = ParseExpr();
    Consume(TokenType::Semicolon, "Expected ';' after return value");
    return std::make_unique<ReturnStmt>(std::move(keyword), std::optional<ExprPtr>(std::move(value)));
  }

  ExprPtr ParseExpr() { return ParseOr(); }

  ExprPtr ParseOr() {
    ExprPtr expr = ParseAnd();
    while (Match(TokenType::Or)) {
      Token op = Previous();
      ExprPtr right = ParseAnd();
      expr = std::make_unique<LogicalExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
  }

  ExprPtr ParseAnd() {
    ExprPtr expr = ParseEquality();
    while (Match(TokenType::And)) {
      Token op = Previous();
      ExprPtr right = ParseEquality();
      expr = std::make_unique<LogicalExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
  }

  ExprPtr ParseEquality() {
    ExprPtr expr = ParseComparison();
    while (Match(TokenType::EqualEqual) || Match(TokenType::BangEqual)) {
      Token op = Previous();
      ExprPtr right = ParseComparison();
      expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
  }

  ExprPtr ParseComparison() {
    ExprPtr expr = ParseTerm();
    while (Match(TokenType::Greater) || Match(TokenType::GreaterEqual) || Match(TokenType::Less) || Match(TokenType::LessEqual)) {
      Token op = Previous();
      ExprPtr right = ParseTerm();
      expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
  }

  ExprPtr ParseTerm() {
    ExprPtr expr = ParseFactor();
    while (Match(TokenType::Plus) || Match(TokenType::Minus)) {
      Token op = Previous();
      ExprPtr right = ParseFactor();
      expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
  }

  ExprPtr ParseFactor() {
    ExprPtr expr = ParseUnary();
    while (Match(TokenType::Star) || Match(TokenType::Slash)) {
      Token op = Previous();
      ExprPtr right = ParseUnary();
      expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
  }

  ExprPtr ParseUnary() {
    if (Match(TokenType::Bang) || Match(TokenType::Minus)) {
      Token op = Previous();
      ExprPtr right = ParseUnary();
      return std::make_unique<UnaryExpr>(std::move(op), std::move(right));
    }
    return ParseCall();
  }

  ExprPtr ParseCall() {
    ExprPtr expr = ParsePrimary();
    while (true) {
      if (Match(TokenType::LeftParen)) {
        Token paren = Previous();
        std::vector<ExprPtr> args;
        if (!Check(TokenType::RightParen)) {
          do {
            args.push_back(ParseExpr());
          } while (Match(TokenType::Comma));
        }
        Consume(TokenType::RightParen, "Expected ')' after arguments");
        expr = std::make_unique<CallExpr>(std::move(expr), std::move(paren), std::move(args));
      } else {
        break;
      }
    }
    return expr;
  }

  ExprPtr ParsePrimary() {
    if (Match(TokenType::Number)) return std::make_unique<LiteralExpr>(LiteralExpr::Kind::Number, Previous().lexeme);
    if (Match(TokenType::String)) return std::make_unique<LiteralExpr>(LiteralExpr::Kind::String, Previous().lexeme);
    if (Match(TokenType::True)) return std::make_unique<LiteralExpr>(LiteralExpr::Kind::Bool, "true");
    if (Match(TokenType::False)) return std::make_unique<LiteralExpr>(LiteralExpr::Kind::Bool, "false");
    if (Match(TokenType::Nil)) return std::make_unique<LiteralExpr>(LiteralExpr::Kind::Nil, "");
    if (Match(TokenType::Identifier)) return std::make_unique<VariableExpr>(Previous());
    if (Match(TokenType::LeftParen)) {
      ExprPtr e = ParseExpr();
      Consume(TokenType::RightParen, "Expected ')' after expression");
      return std::make_unique<GroupingExpr>(std::move(e));
    }
    throw Error(Peek(), "Expected expression");
  }

  bool Match(TokenType t) {
    if (!Check(t)) return false;
    Advance();
    return true;
  }

  bool Check(TokenType t) const {
    if (IsAtEnd()) return t == TokenType::Eof;
    return Peek().type == t;
  }

  Token Advance() {
    if (!IsAtEnd()) current_++;
    return Previous();
  }

  bool IsAtEnd() const { return Peek().type == TokenType::Eof; }

  Token Peek() const { return tokens_[current_]; }

  Token Previous() const { return tokens_[current_ - 1]; }

  bool CheckNext(TokenType t) const {
    if (current_ + 1 >= tokens_.size()) return false;
    return tokens_[current_ + 1].type == t;
  }

  Token Consume(TokenType t, const std::string& message) {
    if (Check(t)) return Advance();
    throw Error(Peek(), message + ", got " + TokenTypeName(Peek().type));
  }

  static ParseError Error(const Token& t, const std::string& message) { return ParseError(t.loc, message); }

  std::vector<Token> tokens_;
  std::size_t current_ = 0;
};

struct RuntimeError : public std::runtime_error {
  explicit RuntimeError(std::string message) : std::runtime_error(std::move(message)) {}
};

struct Value;
struct ListValue;
struct FunctionValue;
struct NativeFunctionValue;

struct ListValue {
  std::vector<Value> items;
};

struct NativeFunctionValue {
  std::string name;
  int arity = -1;
  std::function<Value(const std::vector<Value>&)> fn;
};

struct FunctionValue {
  const FunctionStmt* decl = nullptr;
  std::shared_ptr<struct Environment> closure;
};

struct Value {
  using Variant =
      std::variant<std::monostate, double, bool, std::string, std::shared_ptr<ListValue>, std::shared_ptr<FunctionValue>,
                   std::shared_ptr<NativeFunctionValue>>;
  Variant v;

  static Value Nil() { return Value{std::monostate{}}; }
  static Value Number(double x) { return Value{x}; }
  static Value Bool(bool b) { return Value{b}; }
  static Value Str(std::string s) { return Value{std::move(s)}; }
  static Value List(std::shared_ptr<ListValue> l) { return Value{std::move(l)}; }
  static Value Func(std::shared_ptr<FunctionValue> f) { return Value{std::move(f)}; }
  static Value Native(std::shared_ptr<NativeFunctionValue> nf) { return Value{std::move(nf)}; }

  template <class T>
  explicit Value(T x) : v(std::move(x)) {}
  Value() : v(std::monostate{}) {}
};

static bool IsNil(const Value& v) { return std::holds_alternative<std::monostate>(v.v); }
static bool IsNumber(const Value& v) { return std::holds_alternative<double>(v.v); }
static bool IsBool(const Value& v) { return std::holds_alternative<bool>(v.v); }
static bool IsString(const Value& v) { return std::holds_alternative<std::string>(v.v); }
static bool IsList(const Value& v) { return std::holds_alternative<std::shared_ptr<ListValue>>(v.v); }
static bool IsFunc(const Value& v) { return std::holds_alternative<std::shared_ptr<FunctionValue>>(v.v); }
static bool IsNative(const Value& v) { return std::holds_alternative<std::shared_ptr<NativeFunctionValue>>(v.v); }

static double AsNumber(const Value& v) {
  if (!IsNumber(v)) throw RuntimeError("Expected number");
  return std::get<double>(v.v);
}

static bool AsBool(const Value& v) {
  if (!IsBool(v)) throw RuntimeError("Expected bool");
  return std::get<bool>(v.v);
}

static const std::string& AsString(const Value& v) {
  if (!IsString(v)) throw RuntimeError("Expected string");
  return std::get<std::string>(v.v);
}

static std::shared_ptr<ListValue> AsList(const Value& v) {
  if (!IsList(v)) throw RuntimeError("Expected list");
  return std::get<std::shared_ptr<ListValue>>(v.v);
}

static std::string NumberToString(double x) {
  if (std::isnan(x)) return "nan";
  if (std::isinf(x)) return (x < 0) ? "-inf" : "inf";
  std::ostringstream oss;
  oss << std::setprecision(15) << x;
  std::string s = oss.str();
  if (s.find('.') != std::string::npos) {
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
  }
  return s;
}

static std::string ValueToString(const Value& v);

static bool IsTruthy(const Value& v) {
  if (IsNil(v)) return false;
  if (IsBool(v)) return std::get<bool>(v.v);
  if (IsNumber(v)) return std::get<double>(v.v) != 0.0;
  if (IsString(v)) return !std::get<std::string>(v.v).empty();
  if (IsList(v)) return !std::get<std::shared_ptr<ListValue>>(v.v)->items.empty();
  return true;
}

static bool ValuesEqual(const Value& a, const Value& b) {
  if (a.v.index() != b.v.index()) return false;
  if (IsNil(a)) return true;
  if (IsNumber(a)) return std::get<double>(a.v) == std::get<double>(b.v);
  if (IsBool(a)) return std::get<bool>(a.v) == std::get<bool>(b.v);
  if (IsString(a)) return std::get<std::string>(a.v) == std::get<std::string>(b.v);
  if (IsList(a)) return std::get<std::shared_ptr<ListValue>>(a.v) == std::get<std::shared_ptr<ListValue>>(b.v);
  if (IsFunc(a)) return std::get<std::shared_ptr<FunctionValue>>(a.v) == std::get<std::shared_ptr<FunctionValue>>(b.v);
  if (IsNative(a)) return std::get<std::shared_ptr<NativeFunctionValue>>(a.v) == std::get<std::shared_ptr<NativeFunctionValue>>(b.v);
  return false;
}

static std::string ValueToString(const Value& v) {
  if (IsNil(v)) return "nil";
  if (IsNumber(v)) return NumberToString(std::get<double>(v.v));
  if (IsBool(v)) return std::get<bool>(v.v) ? "true" : "false";
  if (IsString(v)) return std::get<std::string>(v.v);
  if (IsList(v)) return "<list>";
  if (IsFunc(v)) return "<fun>";
  if (IsNative(v)) return "<native>";
  return "nil";
}

struct Environment : std::enable_shared_from_this<Environment> {
  std::unordered_map<std::string, Value> values;
  std::shared_ptr<Environment> parent;

  explicit Environment(std::shared_ptr<Environment> p = nullptr) : parent(std::move(p)) {}

  void Define(const std::string& name, Value v) { values[name] = std::move(v); }

  Value Get(const Token& name) const {
    auto it = values.find(name.lexeme);
    if (it != values.end()) return it->second;
    if (parent) return parent->Get(name);
    throw RuntimeError("Undefined variable: " + name.lexeme);
  }

  void Assign(const Token& name, Value v) {
    auto it = values.find(name.lexeme);
    if (it != values.end()) {
      it->second = std::move(v);
      return;
    }
    if (parent) {
      parent->Assign(name, std::move(v));
      return;
    }
    throw RuntimeError("Undefined variable: " + name.lexeme);
  }
};

struct ReturnSignal {
  Value value;
};

class Interpreter {
 public:
  Interpreter(std::ostream& out, std::ostream& err, std::string input)
      : out_(out), err_(err), globals_(std::make_shared<Environment>()), env_(globals_) {
    globals_->Define("input", Value::Str(std::move(input)));
    InstallBuiltins();
  }

  // Run the interpreter on the provided AST.
  // Returns 0 on success, 1 on runtime error.
  int Run(const std::vector<StmtPtr>& program) {
    try {
      for (const auto& s : program) Execute(s.get());
      return 0;
    } catch (const RuntimeError& e) {
      err_ << "Runtime error: " << e.what() << "\n";
      return 1;
    }
  }

 private:
  // Install built-in native functions into the global scope.
  void InstallBuiltins() {
    auto add = [&](std::string name, int arity, std::function<Value(const std::vector<Value>&)> fn) {
      auto nf = std::make_shared<NativeFunctionValue>();
      nf->name = std::move(name);
      nf->arity = arity;
      nf->fn = std::move(fn);
      globals_->Define(nf->name, Value::Native(nf));
    };

    // Creates a new empty list.
    add("list", 0, [&](const std::vector<Value>&) { return Value::List(std::make_shared<ListValue>()); });
    
    // Pushes an item to the end of a list.
    add("push", 2, [&](const std::vector<Value>& args) {
      auto l = AsList(args[0]);
      l->items.push_back(args[1]);
      return args[0];
    });
    
    // Gets an item from a list by index.
    add("get", 2, [&](const std::vector<Value>& args) {
      auto l = AsList(args[0]);
      int i = static_cast<int>(AsNumber(args[1]));
      if (i < 0 || i >= static_cast<int>(l->items.size())) return Value::Nil();
      return l->items[static_cast<std::size_t>(i)];
    });
    
    // Sets an item in a list by index.
    add("set", 3, [&](const std::vector<Value>& args) {
      auto l = AsList(args[0]);
      int i = static_cast<int>(AsNumber(args[1]));
      if (i < 0 || i >= static_cast<int>(l->items.size())) throw RuntimeError("Index out of range");
      l->items[static_cast<std::size_t>(i)] = args[2];
      return args[0];
    });
    
    // Returns the length of a string or list.
    add("len", 1, [&](const std::vector<Value>& args) {
      if (IsString(args[0])) return Value::Number(static_cast<double>(AsString(args[0]).size()));
      if (IsList(args[0])) return Value::Number(static_cast<double>(AsList(args[0])->items.size()));
      throw RuntimeError("len() expects string or list");
    });
    // Returns a substring of a string.
    add("substr", 3, [&](const std::vector<Value>& args) {
      const std::string& s = AsString(args[0]);
      int start = static_cast<int>(AsNumber(args[1]));
      int count = static_cast<int>(AsNumber(args[2]));
      if (count <= 0) return Value::Str("");
      if (start < 0) start = 0;
      if (start > static_cast<int>(s.size())) start = static_cast<int>(s.size());
      int end = start + count;
      if (end > static_cast<int>(s.size())) end = static_cast<int>(s.size());
      return Value::Str(s.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start)));
    });

    // Returns the character at a specific index in a string.
    add("char_at", 2, [&](const std::vector<Value>& args) {
      const std::string& s = AsString(args[0]);
      int i = static_cast<int>(AsNumber(args[1]));
      if (i < 0 || i >= static_cast<int>(s.size())) return Value::Str("");
      return Value::Str(s.substr(static_cast<std::size_t>(i), 1));
    });

    // Converts any value to a string representation.
    add("to_string", 1, [&](const std::vector<Value>& args) { return Value::Str(ValueToString(args[0])); });
    
    // Writes a string to standard output.
    add("write", 1, [&](const std::vector<Value>& args) {
      out_ << ValueToString(args[0]);
      out_.flush();
      return Value::Nil();
    });

    // Checks if a string contains only digits.
    add("is_digit", 1, [&](const std::vector<Value>& args) {
      const std::string& s = AsString(args[0]);
      if (s.size() != 1) return Value::Bool(false);
      return Value::Bool(std::isdigit(static_cast<unsigned char>(s[0])) != 0);
    });

    // Checks if a string contains only alphabetic characters.
    add("is_alpha", 1, [&](const std::vector<Value>& args) {
      const std::string& s = AsString(args[0]);
      if (s.size() != 1) return Value::Bool(false);
      return Value::Bool(std::isalpha(static_cast<unsigned char>(s[0])) != 0 || s[0] == '_');
    });

    // Checks if a string contains only alphanumeric characters.
    add("is_alnum", 1, [&](const std::vector<Value>& args) {
      const std::string& s = AsString(args[0]);
      if (s.size() != 1) return Value::Bool(false);
      return Value::Bool(std::isalnum(static_cast<unsigned char>(s[0])) != 0 || s[0] == '_');
    });
  }

  // Imports a module from a file.
  void ImportModule(const Token& moduleTok) {
    std::string name = moduleTok.lexeme;
    if (imported_modules_.find(name) != imported_modules_.end()) return;
    imported_modules_[name] = true;

    std::string path;
    if (!name.empty() && (name[0] == '/' || name.rfind("./", 0) == 0 || name.rfind("../", 0) == 0)) {
      path = name;
    } else {
      path = module_base_dir_ + "/" + name;
    }
    if (path.size() < 3 || path.substr(path.size() - 3) != ".pt") path += ".pt";

    std::ifstream f(path);
    if (!f) {
      imported_modules_.erase(name);
      throw RuntimeError("Failed to import module: " + name);
    }

    try {
      std::ostringstream ss;
      ss << f.rdbuf();
      std::string source = ss.str();

      Lexer lexer(std::move(source));
      std::vector<Token> tokens = lexer.LexAll();
      for (const auto& t : tokens) {
        if (t.type == TokenType::Invalid) {
          throw RuntimeError("Lex error importing module: " + name);
        }
      }

      Parser parser(std::move(tokens));
      std::vector<StmtPtr> program = parser.ParseProgram();
      imported_programs_[name] = std::move(program);
      const std::vector<StmtPtr>& kept = imported_programs_[name];

      std::shared_ptr<Environment> previous = env_;
      env_ = globals_;
      try {
        for (const auto& s : kept) Execute(s.get());
      } catch (...) {
        env_ = previous;
        throw;
      }
      env_ = previous;
    } catch (const ParseError& e) {
      imported_modules_.erase(name);
      imported_programs_.erase(name);
      throw RuntimeError(std::string(e.what()));
    } catch (...) {
      imported_modules_.erase(name);
      imported_programs_.erase(name);
      throw;
    }
  }

  // Executes a single statement.
  void Execute(const Stmt* stmt) {
    if (auto s = dynamic_cast<const ImportStmt*>(stmt)) {
      ImportModule(s->module);
      return;
    }
    if (auto s = dynamic_cast<const LetStmt*>(stmt)) {
      Value v = Evaluate(s->init.get());
      env_->Define(s->name.lexeme, std::move(v));
      return;
    }
    if (auto s = dynamic_cast<const AssignStmt*>(stmt)) {
      Value v = Evaluate(s->value.get());
      env_->Assign(s->name, std::move(v));
      return;
    }
    if (auto s = dynamic_cast<const PrintStmt*>(stmt)) {
      Value v = Evaluate(s->expr.get());
      out_ << ValueToString(v) << "\n";
      return;
    }
    if (auto s = dynamic_cast<const ExprStmt*>(stmt)) {
      (void)Evaluate(s->expr.get());
      return;
    }
    if (auto s = dynamic_cast<const BlockStmt*>(stmt)) {
      ExecuteBlock(s->statements, std::make_shared<Environment>(env_));
      return;
    }
    if (auto s = dynamic_cast<const IfStmt*>(stmt)) {
      if (IsTruthy(Evaluate(s->condition.get()))) {
        Execute(s->thenBranch.get());
      } else if (s->elseBranch.has_value()) {
        Execute((*s->elseBranch).get());
      }
      return;
    }
    if (auto s = dynamic_cast<const WhileStmt*>(stmt)) {
      while (IsTruthy(Evaluate(s->condition.get()))) Execute(s->body.get());
      return;
    }
    if (auto s = dynamic_cast<const FunctionStmt*>(stmt)) {
      auto f = std::make_shared<FunctionValue>();
      f->decl = s;
      f->closure = env_;
      env_->Define(s->name.lexeme, Value::Func(std::move(f)));
      return;
    }
    if (auto s = dynamic_cast<const ReturnStmt*>(stmt)) {
      Value v = Value::Nil();
      if (s->value.has_value()) v = Evaluate((*s->value).get());
      throw ReturnSignal{std::move(v)};
    }
    throw RuntimeError("Unknown statement");
  }

  // Executes a block of statements in a new environment.
  void ExecuteBlock(const std::vector<StmtPtr>& statements, std::shared_ptr<Environment> newEnv) {
    std::shared_ptr<Environment> previous = env_;
    env_ = std::move(newEnv);
    try {
      for (const auto& s : statements) Execute(s.get());
    } catch (...) {
      env_ = previous;
      throw;
    }
    env_ = previous;
  }

  // Evaluates an expression and returns a value.
  Value Evaluate(const Expr* expr) {
    if (auto e = dynamic_cast<const LiteralExpr*>(expr)) {
      switch (e->kind) {
        case LiteralExpr::Kind::Number: return Value::Number(std::strtod(e->value.c_str(), nullptr));
        case LiteralExpr::Kind::String: return Value::Str(e->value);
        case LiteralExpr::Kind::Bool: return Value::Bool(e->value == "true");
        case LiteralExpr::Kind::Nil: return Value::Nil();
      }
    }
    if (auto e = dynamic_cast<const VariableExpr*>(expr)) return env_->Get(e->name);
    if (auto e = dynamic_cast<const GroupingExpr*>(expr)) return Evaluate(e->expr.get());
    if (auto e = dynamic_cast<const UnaryExpr*>(expr)) {
      Value right = Evaluate(e->right.get());
      if (e->op.type == TokenType::Minus) {
        return Value::Number(-AsNumber(right));
      }
      if (e->op.type == TokenType::Bang) return Value::Bool(!IsTruthy(right));
      throw RuntimeError("Unknown unary operator");
    }
    if (auto e = dynamic_cast<const LogicalExpr*>(expr)) {
      Value left = Evaluate(e->left.get());
      if (e->op.type == TokenType::Or) {
        if (IsTruthy(left)) return left;
        return Evaluate(e->right.get());
      }
      if (e->op.type == TokenType::And) {
        if (!IsTruthy(left)) return left;
        return Evaluate(e->right.get());
      }
      throw RuntimeError("Unknown logical operator");
    }
    if (auto e = dynamic_cast<const BinaryExpr*>(expr)) {
      Value left = Evaluate(e->left.get());
      Value right = Evaluate(e->right.get());
      switch (e->op.type) {
        case TokenType::Plus:
          if (IsNumber(left) && IsNumber(right)) return Value::Number(AsNumber(left) + AsNumber(right));
          if (IsString(left) && IsString(right)) return Value::Str(AsString(left) + AsString(right));
          throw RuntimeError("Operator + expects two numbers or two strings");
        case TokenType::Minus: return Value::Number(AsNumber(left) - AsNumber(right));
        case TokenType::Star: return Value::Number(AsNumber(left) * AsNumber(right));
        case TokenType::Slash: return Value::Number(AsNumber(left) / AsNumber(right));
        case TokenType::Greater: return Value::Bool(AsNumber(left) > AsNumber(right));
        case TokenType::GreaterEqual: return Value::Bool(AsNumber(left) >= AsNumber(right));
        case TokenType::Less: return Value::Bool(AsNumber(left) < AsNumber(right));
        case TokenType::LessEqual: return Value::Bool(AsNumber(left) <= AsNumber(right));
        case TokenType::EqualEqual: return Value::Bool(ValuesEqual(left, right));
        case TokenType::BangEqual: return Value::Bool(!ValuesEqual(left, right));
        default: throw RuntimeError("Unknown binary operator");
      }
    }
    if (auto e = dynamic_cast<const CallExpr*>(expr)) {
      Value callee = Evaluate(e->callee.get());
      std::vector<Value> args;
      args.reserve(e->args.size());
      for (const auto& a : e->args) args.push_back(Evaluate(a.get()));
      return Call(std::move(callee), args);
    }
    throw RuntimeError("Unknown expression");
  }

  // Calls a function (native or user-defined).
  Value Call(Value callee, const std::vector<Value>& args) {
    if (IsNative(callee)) {
      auto nf = std::get<std::shared_ptr<NativeFunctionValue>>(callee.v);
      if (nf->arity >= 0 && static_cast<int>(args.size()) != nf->arity) {
        throw RuntimeError("Arity mismatch calling " + nf->name);
      }
      return nf->fn(args);
    }
    if (IsFunc(callee)) {
      auto f = std::get<std::shared_ptr<FunctionValue>>(callee.v);
      const FunctionStmt* decl = f->decl;
      if (static_cast<int>(args.size()) != static_cast<int>(decl->params.size())) {
        throw RuntimeError("Arity mismatch calling " + decl->name.lexeme);
      }
      auto callEnv = std::make_shared<Environment>(f->closure);
      for (std::size_t i = 0; i < decl->params.size(); i++) {
        callEnv->Define(decl->params[i].lexeme, args[i]);
      }
      try {
        ExecuteBlock(decl->body, std::move(callEnv));
      } catch (const ReturnSignal& r) {
        return r.value;
      }
      return Value::Nil();
    }
    throw RuntimeError("Can only call functions");
  }

  std::ostream& out_;
  std::ostream& err_;
  std::shared_ptr<Environment> globals_;
  std::shared_ptr<Environment> env_;
  std::unordered_map<std::string, bool> imported_modules_;
  std::unordered_map<std::string, std::vector<StmtPtr>> imported_programs_;
  std::string module_base_dir_ = "potatos";
};

static std::string ReadAll(std::istream& in) {
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static std::string ReadFile(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("Failed to open file: " + path);
  return ReadAll(f);
}

static int ParseOnly(const std::string& source, std::ostream& out, std::ostream& err) {
  Lexer lexer(source);
  std::vector<Token> tokens = lexer.LexAll();
  for (const auto& t : tokens) {
    if (t.type == TokenType::Invalid) {
      err << "Lex error at " << t.loc.line << ":" << t.loc.column << ": " << t.lexeme << "\n";
      return 1;
    }
  }

  try {
    Parser parser(std::move(tokens));
    std::vector<StmtPtr> program = parser.ParseProgram();
    out << "(program";
    for (const auto& s : program) {
      out << " ";
      s->Print(out);
    }
    out << ")\n";
    return 0;
  } catch (const ParseError& e) {
    err << e.what() << "\n";
    return 1;
  }
}

static int RunScript(const std::string& scriptSource, const std::string& input, std::ostream& out, std::ostream& err) {
  Lexer lexer(scriptSource);
  std::vector<Token> tokens = lexer.LexAll();
  for (const auto& t : tokens) {
    if (t.type == TokenType::Invalid) {
      err << "Lex error at " << t.loc.line << ":" << t.loc.column << ": " << t.lexeme << "\n";
      return 1;
    }
  }
  try {
    Parser parser(std::move(tokens));
    std::vector<StmtPtr> program = parser.ParseProgram();
    Interpreter interp(out, err, input);
    return interp.Run(program);
  } catch (const ParseError& e) {
    err << e.what() << "\n";
    return 1;
  }
}

}  // namespace potatolang

int main(int argc, char** argv) {
  try {
    if (argc >= 2 && std::string(argv[1]) == "--run") {
      if (argc < 3) throw std::runtime_error("Usage: potatolang --run <script.pt> [input.pt]");
      std::string script = potatolang::ReadFile(argv[2]);
      std::string input;
      if (argc >= 4) {
        if (std::string(argv[3]) == "-") {
          input = potatolang::ReadAll(std::cin);
        } else {
          input = potatolang::ReadFile(argv[3]);
        }
      } else {
        input = "";
      }
      return potatolang::RunScript(script, input, std::cout, std::cerr);
    }
    if (argc >= 2) return potatolang::ParseOnly(potatolang::ReadFile(argv[1]), std::cout, std::cerr);
    return potatolang::ParseOnly(potatolang::ReadAll(std::cin), std::cout, std::cerr);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
