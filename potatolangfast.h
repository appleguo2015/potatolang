#pragma once
// aPpLegUo

// ============================================================================
// POTATOLANG FAST - The "Elementary School" Edition
// Simple, Fast, and Functional.
// ============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <random>
#include <thread>
#include <iomanip>
#include <cctype>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

namespace potatolang {

// ----------------------------------------------------------------------------
// 1. DATA STRUCTURES
// ----------------------------------------------------------------------------

struct Token {
    std::string type;    // "Number", "String", "Identifier", etc.
    std::string lexeme;  // The actual text
    int line;
};

struct Node {
    std::string kind;    // "Binary", "If", "Literal", etc.
    Token token;
    std::string string_val;
    double number_val = 0.0;
    std::shared_ptr<Node> lhs, rhs, body;
    std::vector<std::shared_ptr<Node>> children;

    void Print(std::ostream& out) const {
        if (kind == "Literal") {
            if (token.type == "String") out << '"' << string_val << '"';
            else if (token.type == "Nil") out << "nil";
            else if (token.type == "Bool") out << (number_val ? "true" : "false");
            else out << number_val;
        } else if (kind == "Variable") out << token.lexeme;
        else if (kind == "Block") {
            out << "(block"; for (auto& c : children) { out << " "; c->Print(out); } out << ")";
        } else {
            out << "(" << token.lexeme;
            if (lhs) { out << " "; lhs->Print(out); }
            if (rhs) { out << " "; rhs->Print(out); }
            if (body) { out << " "; body->Print(out); }
            for (auto& c : children) { out << " "; c->Print(out); }
            out << ")";
        }
    }
};

struct Value;
struct ListValue { std::vector<Value> items; };
struct FunctionValue { std::shared_ptr<Node> decl; std::shared_ptr<struct Environment> closure; };
struct NativeFunctionValue { std::string name; std::function<Value(const std::vector<Value>&)> fn; };

struct Value {
    std::variant<double, bool, std::string, std::shared_ptr<ListValue>, std::shared_ptr<FunctionValue>, std::shared_ptr<NativeFunctionValue>, std::nullptr_t> v;

    static Value Nil() { return {nullptr}; }
    static Value Bool(bool b) { return {b}; }
    static Value Number(double d) { return {d}; }
    static Value Str(std::string s) { return {s}; }
    static Value List(std::shared_ptr<ListValue> l) { return {l}; }
    
    bool is_nil() const { return std::holds_alternative<std::nullptr_t>(v); }
    bool is_bool() const { return std::holds_alternative<bool>(v); }
    bool is_num() const { return std::holds_alternative<double>(v); }
    bool is_str() const { return std::holds_alternative<std::string>(v); }
    bool is_list() const { return std::holds_alternative<std::shared_ptr<ListValue>>(v); }
    
    double as_num() const { return is_num() ? std::get<double>(v) : 0.0; }
    bool as_bool() const { return is_bool() ? std::get<bool>(v) : false; }
    std::string as_str() const { return is_str() ? std::get<std::string>(v) : ""; }
    
    std::string to_string() const {
        if (is_nil()) return "nil";
        if (is_bool()) return as_bool() ? "true" : "false";
        if (is_num()) {
            std::string s = std::to_string(as_num());
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s.pop_back();
            return s;
        }
        if (is_str()) return as_str();
        if (is_list()) return "<list>";
        return "<function>";
    }
};

// ----------------------------------------------------------------------------
// 2. LEXER
// ----------------------------------------------------------------------------

class Lexer {
    std::string src; int pos = 0, line = 1;
public:
    explicit Lexer(std::string s) : src(std::move(s)) {}
    std::vector<Token> Scan() {
        std::vector<Token> tokens;
        while (pos < src.size()) {
            char c = src[pos];
            if (isspace(c)) { if (c == '\n') line++; pos++; }
            else if (c == '/' && pos+1 < src.size() && src[pos+1] == '/') { while (pos < src.size() && src[pos] != '\n') pos++; }
            else if (isdigit(c)) {
                int start = pos; while (pos < src.size() && (isdigit(src[pos]) || src[pos] == '.')) pos++;
                tokens.push_back({"Number", src.substr(start, pos - start), line});
            } else if (isalpha(c) || c == '_') {
                int start = pos; while (pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')) pos++;
                std::string t = src.substr(start, pos - start);
                std::string type = "Identifier";
                if (t=="if"||t=="else"||t=="while"||t=="fun"||t=="return"||t=="true"||t=="false"||t=="nil"||t=="let"||t=="print"||t=="import"||t=="and"||t=="or") type = t;
                tokens.push_back({type, t, line});
            } else if (c == '"') {
                pos++; std::string val;
                while (pos < src.size() && src[pos] != '"') {
                    if (src[pos] == '\\' && pos+1 < src.size()) {
                        pos++; char e = src[pos];
                        if (e=='n') val+='\n'; else if (e=='t') val+='\t'; else val+=e;
                    } else val += src[pos];
                    pos++;
                }
                if (pos < src.size()) pos++; tokens.push_back({"String", val, line});
            } else {
                std::string s(1, c); pos++;
                if (pos < src.size() && src[pos] == '=') { if (c=='!'||c=='='||c=='<'||c=='>') { s+='='; pos++; } }
                tokens.push_back({"Symbol", s, line});
            }
        }
        tokens.push_back({"EOF", "", line});
        return tokens;
    }
};

// ----------------------------------------------------------------------------
// 3. PARSER
// ----------------------------------------------------------------------------

class Parser {
    std::vector<Token> tokens; int current = 0;
    Token peek() { return tokens[current]; }
    Token prev() { return tokens[current - 1]; }
    bool match(std::string type) { if (check(type)) { current++; return true; } return false; }
    bool check(std::string type) {
        if (peek().type == "EOF") return false;
        if (peek().type == type || (peek().type == "Symbol" && peek().lexeme == type)) return true;
        if (peek().type == "Identifier" && type == "Identifier") return true;
        return false;
    }
    Token consume(std::string type, std::string err) { if (check(type)) return tokens[current++]; throw std::runtime_error(err); }

public:
    explicit Parser(std::vector<Token> t) : tokens(std::move(t)) {}
    std::vector<std::shared_ptr<Node>> Parse() {
        std::vector<std::shared_ptr<Node>> stmts;
        while (peek().type != "EOF") stmts.push_back(Declaration());
        return stmts;
    }
    std::shared_ptr<Node> Declaration() {
        if (match("fun")) {
            Token name = consume("Identifier", "Expect fun name");
            consume("(", "Expect '('");
            auto n = std::make_shared<Node>(); n->kind = "Function"; n->token = name;
            if (!check(")")) do {
                Token p = consume("Identifier", "Expect param");
                auto v = std::make_shared<Node>(); v->kind = "Variable"; v->token = p;
                n->children.push_back(v);
            } while (match(","));
            consume(")", "Expect ')'"); consume("{", "Expect '{'"); n->body = Block();
            return n;
        }
        if (match("let")) {
            Token name = consume("Identifier", "Expect var name");
            auto n = std::make_shared<Node>(); n->kind = "Let"; n->token = name;
            if (match("=")) n->lhs = Expression();
            consume(";", "Expect ';'"); return n;
        }
        return Statement();
    }
    std::shared_ptr<Node> Statement() {
        if (match("print")) { auto n = std::make_shared<Node>(); n->kind = "Print"; n->lhs = Expression(); consume(";", ";"); return n; }
        if (match("return")) { auto n = std::make_shared<Node>(); n->kind = "Return"; if (!check(";")) n->lhs = Expression(); consume(";", ";"); return n; }
        if (match("while")) { consume("(", "("); auto c = Expression(); consume(")", ")"); auto n = std::make_shared<Node>(); n->kind = "While"; n->lhs = c; n->body = Statement(); return n; }
        if (match("if")) { consume("(", "("); auto c = Expression(); consume(")", ")"); auto n = std::make_shared<Node>(); n->kind = "If"; n->children.push_back(c); n->lhs = Statement(); if (match("else")) n->rhs = Statement(); return n; }
        if (match("{")) return Block();
        if (match("import")) { Token t = consume("String", "Expect import string"); consume(";", ";"); auto n = std::make_shared<Node>(); n->kind = "Import"; n->string_val = t.lexeme; return n; }
        auto n = std::make_shared<Node>(); n->kind = "ExprStmt"; n->lhs = Expression(); consume(";", ";"); return n;
    }
    std::shared_ptr<Node> Block() {
        auto n = std::make_shared<Node>(); n->kind = "Block";
        while (!check("}") && !check("EOF")) n->children.push_back(Declaration());
        consume("}", "Expect '}'"); return n;
    }
    std::shared_ptr<Node> Expression() { return Assignment(); }
    std::shared_ptr<Node> Assignment() {
        auto e = Or();
        if (match("=")) {
            if (e->kind != "Variable") throw std::runtime_error("Invalid assignment target");
            auto n = std::make_shared<Node>(); n->kind = "Assign"; n->token = e->token; n->lhs = Assignment();
            return n;
        }
        return e;
    }
    std::shared_ptr<Node> Or() { auto e = And(); while (match("or")) { auto n = std::make_shared<Node>(); n->kind = "Logical"; n->token = prev(); n->lhs = e; n->rhs = And(); e = n; } return e; }
    std::shared_ptr<Node> And() { auto e = Equality(); while (match("and")) { auto n = std::make_shared<Node>(); n->kind = "Logical"; n->token = prev(); n->lhs = e; n->rhs = Equality(); e = n; } return e; }
    std::shared_ptr<Node> Equality() { auto e = Comparison(); while (match("!=") || match("==")) { auto n = std::make_shared<Node>(); n->kind = "Binary"; n->token = prev(); n->lhs = e; n->rhs = Comparison(); e = n; } return e; }
    std::shared_ptr<Node> Comparison() { auto e = Term(); while (match(">") || match(">=") || match("<") || match("<=")) { auto n = std::make_shared<Node>(); n->kind = "Binary"; n->token = prev(); n->lhs = e; n->rhs = Term(); e = n; } return e; }
    std::shared_ptr<Node> Term() { auto e = Factor(); while (match("-") || match("+")) { auto n = std::make_shared<Node>(); n->kind = "Binary"; n->token = prev(); n->lhs = e; n->rhs = Factor(); e = n; } return e; }
    std::shared_ptr<Node> Factor() { auto e = Unary(); while (match("/") || match("*")) { auto n = std::make_shared<Node>(); n->kind = "Binary"; n->token = prev(); n->lhs = e; n->rhs = Unary(); e = n; } return e; }
    std::shared_ptr<Node> Unary() { if (match("!") || match("-")) { auto n = std::make_shared<Node>(); n->kind = "Unary"; n->token = prev(); n->lhs = Unary(); return n; } return Call(); }
    std::shared_ptr<Node> Call() {
        auto e = Primary();
        while (match("(")) {
            auto n = std::make_shared<Node>(); n->kind = "Call"; n->lhs = e;
            if (!check(")")) do { n->children.push_back(Expression()); } while (match(","));
            consume(")", ")"); e = n;
        }
        return e;
    }
    std::shared_ptr<Node> Primary() {
        if (match("false")) { auto n = std::make_shared<Node>(); n->kind = "Literal"; n->token = prev(); n->token.type = "Bool"; n->number_val = 0; return n; }
        if (match("true")) { auto n = std::make_shared<Node>(); n->kind = "Literal"; n->token = prev(); n->token.type = "Bool"; n->number_val = 1; return n; }
        if (match("nil")) { auto n = std::make_shared<Node>(); n->kind = "Literal"; n->token = prev(); n->token.type = "Nil"; return n; }
        if (match("Number")) { auto n = std::make_shared<Node>(); n->kind = "Literal"; n->token = prev(); n->number_val = std::stod(prev().lexeme); return n; }
        if (match("String")) { auto n = std::make_shared<Node>(); n->kind = "Literal"; n->token = prev(); n->string_val = prev().lexeme; return n; }
        if (match("Identifier")) { auto n = std::make_shared<Node>(); n->kind = "Variable"; n->token = prev(); return n; }
        if (match("(")) { auto e = Expression(); consume(")", ")"); return e; }
        throw std::runtime_error("Expect expression");
    }
};

// ----------------------------------------------------------------------------
// 4. INTERPRETER
// ----------------------------------------------------------------------------

struct Environment {
    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> parent;
    Environment(std::shared_ptr<Environment> p = nullptr) : parent(p) {}
    void define(std::string n, Value v) { values[n] = v; }
    Value get(std::string n) { if (values.count(n)) return values[n]; if (parent) return parent->get(n); throw std::runtime_error("Undefined: " + n); }
    void assign(std::string n, Value v) { if (values.count(n)) { values[n] = v; return; } if (parent) { parent->assign(n, v); return; } throw std::runtime_error("Undefined: " + n); }
};

class Interpreter {
public:
    std::shared_ptr<Environment> globals = std::make_shared<Environment>(), env = globals;
    std::ostream &out, &err;
    static SDL_Window* win; static SDL_Renderer* ren; static const unsigned char font[];

    Interpreter(std::ostream& o, std::ostream& e, std::string in) : out(o), err(e) { globals->define("input", Value::Str(in)); InstallBuiltins(); }
    void Run(const std::vector<std::shared_ptr<Node>>& stmts) { try { for (auto& s : stmts) Execute(s); } catch (const std::exception& e) { err << "Runtime Error: " << e.what() << "\n"; } catch (Value v) { /* Uncaught return */ } }

    Value Evaluate(std::shared_ptr<Node> n) {
        if (n->kind == "Literal") {
            if (n->token.type == "Number") return Value::Number(n->number_val);
            if (n->token.type == "String") return Value::Str(n->string_val);
            if (n->token.type == "Bool") return Value::Bool(n->number_val);
            return Value::Nil();
        }
        if (n->kind == "Variable") return env->get(n->token.lexeme);
        if (n->kind == "Assign") { Value v = Evaluate(n->lhs); env->assign(n->token.lexeme, v); return v; }
        if (n->kind == "Unary") {
            Value r = Evaluate(n->lhs);
            if (n->token.lexeme == "-") return Value::Number(-r.as_num());
            if (n->token.lexeme == "!") return Value::Bool(r.is_nil() || (r.is_bool() && !r.as_bool()));
        }
        if (n->kind == "Binary") {
            Value l = Evaluate(n->lhs), r = Evaluate(n->rhs);
            std::string op = n->token.lexeme;
            if (op == "+") return (l.is_str() && r.is_str()) ? Value::Str(l.as_str() + r.as_str()) : Value::Number(l.as_num() + r.as_num());
            if (op == "-") return Value::Number(l.as_num() - r.as_num());
            if (op == "*") {
                 if (l.is_str() && r.is_num()) { std::string s; for(int i=0;i<(int)r.as_num();++i) s+=l.as_str(); return Value::Str(s); }
                 return Value::Number(l.as_num() * r.as_num());
            }
            if (op == "/") return Value::Number(l.as_num() / r.as_num());
            if (op == ">") return Value::Bool(l.as_num() > r.as_num());
            if (op == "<") return Value::Bool(l.as_num() < r.as_num());
            if (op == "==") return Value::Bool(l.is_num()&&r.is_num() ? l.as_num()==r.as_num() : (l.is_str()&&r.is_str() ? l.as_str()==r.as_str() : l.is_nil()&&r.is_nil()));
            if (op == "!=") return Value::Bool(!Evaluate(n).as_bool()); // This recursion is wrong because we need to reconstruct check. Simplified:
            if (op == "!=") return Value::Bool(!(l.is_num()&&r.is_num() ? l.as_num()==r.as_num() : (l.is_str()&&r.is_str() ? l.as_str()==r.as_str() : l.is_nil()&&r.is_nil())));
        }
        if (n->kind == "Call") {
            Value c = Evaluate(n->lhs); std::vector<Value> args; for (auto& ch : n->children) args.push_back(Evaluate(ch));
            if (std::holds_alternative<std::shared_ptr<NativeFunctionValue>>(c.v)) return std::get<std::shared_ptr<NativeFunctionValue>>(c.v)->fn(args);
            if (std::holds_alternative<std::shared_ptr<FunctionValue>>(c.v)) {
                auto f = std::get<std::shared_ptr<FunctionValue>>(c.v);
                auto pe = env; env = std::make_shared<Environment>(f->closure);
                for (size_t i = 0; i < f->decl->children.size(); i++) env->define(f->decl->children[i]->token.lexeme, args[i]);
                try { Execute(f->decl->body); } catch (Value r) { env = pe; return r; }
                env = pe; return Value::Nil();
            }
        }
        if (n->kind == "Logical") {
             Value l = Evaluate(n->lhs);
             if (n->token.lexeme == "or") { if (!l.is_nil() && !(l.is_bool() && !l.as_bool())) return l; }
             else { if (l.is_nil() || (l.is_bool() && !l.as_bool())) return l; }
             return Evaluate(n->rhs);
        }
        return Value::Nil();
    }

    void Execute(std::shared_ptr<Node> n) {
        if (n->kind == "Print") out << Evaluate(n->lhs).to_string() << "\n";
        else if (n->kind == "ExprStmt") Evaluate(n->lhs);
        else if (n->kind == "Let") env->define(n->token.lexeme, n->lhs ? Evaluate(n->lhs) : Value::Nil());
        else if (n->kind == "Block") { auto pe = env; env = std::make_shared<Environment>(pe); for (auto& c : n->children) Execute(c); env = pe; }
        else if (n->kind == "If") { Value c = Evaluate(n->children[0]); if (!c.is_nil() && !(c.is_bool() && !c.as_bool())) Execute(n->lhs); else if (n->rhs) Execute(n->rhs); }
        else if (n->kind == "While") { while (true) { Value c = Evaluate(n->lhs); if (c.is_nil() || (c.is_bool() && !c.as_bool())) break; Execute(n->body); } }
        else if (n->kind == "Function") { auto f = std::make_shared<FunctionValue>(); f->decl = n; f->closure = env; env->define(n->token.lexeme, Value{f}); }
        else if (n->kind == "Return") throw n->lhs ? Evaluate(n->lhs) : Value::Nil();
        else if (n->kind == "Import") {
             std::ifstream f(n->string_val); if (!f) return;
             std::stringstream b; b << f.rdbuf(); Lexer l(b.str()); Parser p(l.Scan());
             for (auto& s : p.Parse()) Execute(s);
        }
    }

    void InstallBuiltins() {
        auto d = [&](std::string n, NativeFunc f) { auto nf = std::make_shared<NativeFunctionValue>(); nf->name = n; nf->fn = f; globals->define(n, Value{nf}); };
        d("clock", [](auto){ return Value::Number(clock() / (double)CLOCKS_PER_SEC); });
        d("read_line", [](auto){ std::string s; std::getline(std::cin, s); return Value::Str(s); });
        d("list", [](auto){ return Value::List(std::make_shared<ListValue>()); });
        d("push", [](auto a){ std::get<std::shared_ptr<ListValue>>(a[0].v)->items.push_back(a[1]); return a[0]; });
        d("get", [](auto a){ auto& l = std::get<std::shared_ptr<ListValue>>(a[0].v)->items; int i=(int)a[1].as_num(); return (i>=0&&i<l.size())?l[i]:Value::Nil(); });
        d("set", [](auto a){ auto& l = std::get<std::shared_ptr<ListValue>>(a[0].v)->items; int i=(int)a[1].as_num(); if(i>=0&&i<l.size()) l[i]=a[2]; return a[0]; });
        d("remove_at", [](auto a){ auto& l = std::get<std::shared_ptr<ListValue>>(a[0].v)->items; int i=(int)a[1].as_num(); if(i>=0&&i<l.size()) l.erase(l.begin()+i); return Value::Nil(); });
        d("len", [](auto a){ return a[0].is_str() ? Value::Number(a[0].as_str().size()) : (a[0].is_list() ? Value::Number(std::get<std::shared_ptr<ListValue>>(a[0].v)->items.size()) : Value::Number(0)); });
        d("substr", [](auto a){ return Value::Str(a[0].as_str().substr((int)a[1].as_num(), (int)a[2].as_num())); });
        d("char_at", [](auto a){ return Value::Str(std::string(1, a[0].as_str().at((int)a[1].as_num()))); });
        d("to_string", [](auto a){ return Value::Str(a[0].to_string()); });
        d("write", [&](auto a){ out << a[0].to_string(); return Value::Nil(); });
        d("int", [](auto a){ return Value::Number((int)a[0].as_num()); });
        d("random", [](auto){ return Value::Number((double)rand()/RAND_MAX); });
        d("char", [](auto a){ return Value::Str(std::string(1, (char)a[0].as_num())); });
        d("system", [](auto a){ return Value::Number(system(a[0].as_str().c_str())); });
        d("exec", [](auto a){ char b[128]; std::string r; FILE* p = popen(a[0].as_str().c_str(), "r"); if(p){while(fgets(b,128,p))r+=b; pclose(p);} return Value::Str(r); });
        d("sleep", [](auto a){ std::this_thread::sleep_for(std::chrono::milliseconds((int)a[0].as_num())); return Value::Nil(); });
        d("_file_exists", [](auto a){ std::ifstream f(a[0].as_str()); return Value::Bool(f.good()); });
        d("_file_read", [](auto a){ std::ifstream f(a[0].as_str()); std::stringstream s; s<<f.rdbuf(); return Value::Str(s.str()); });
        d("_file_write", [](auto a){ std::ofstream f(a[0].as_str()); f<<a[1].as_str(); return Value::Bool(true); });

        // Graphics
        d("graphics_init", [&](auto a){ 
            if (SDL_Init(SDL_INIT_VIDEO)<0) return Value::Bool(false);
            win = SDL_CreateWindow(a[2].as_str().c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, a[0].as_num(), a[1].as_num(), SDL_WINDOW_SHOWN);
            ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
            return Value::Bool(ren != nullptr);
        });
        d("graphics_clear", [&](auto){ SDL_RenderClear(ren); return Value::Nil(); });
        d("graphics_color", [&](auto a){ SDL_SetRenderDrawColor(ren, a[0].as_num(), a[1].as_num(), a[2].as_num(), 255); return Value::Nil(); });
        d("graphics_rect", [&](auto a){ SDL_Rect r={(int)a[0].as_num(), (int)a[1].as_num(), (int)a[2].as_num(), (int)a[3].as_num()}; SDL_RenderFillRect(ren, &r); return Value::Nil(); });
        d("graphics_present", [&](auto){ SDL_RenderPresent(ren); return Value::Nil(); });
        d("graphics_poll", [&](auto){ SDL_Event e; if(SDL_PollEvent(&e)){ if(e.type==SDL_QUIT) return Value::Str("quit"); if(e.type==SDL_TEXTINPUT) return Value::Str("text:"+std::string(e.text.text)); if(e.type==SDL_KEYDOWN) return Value::Str(SDL_GetKeyName(e.key.keysym.sym)); } return Value::Nil(); });
        d("graphics_draw_text", [&](auto a){
            int x=(int)a[0].as_num(), y=(int)a[1].as_num();
            for(char c : a[2].as_str()) {
                unsigned char uc = c; if(uc<32||uc>127) uc=127;
                int idx = (uc-32)*5;
                for(int col=0;col<5;++col) {
                    unsigned char d = font[idx+col];
                    for(int row=0;row<7;++row) if((d>>row)&1) SDL_RenderDrawPoint(ren, x+col, y+row);
                }
                x+=6;
            }
            return Value::Nil();
        });
    }
};

SDL_Window* Interpreter::win = nullptr;
SDL_Renderer* Interpreter::ren = nullptr;
const unsigned char Interpreter::font[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5F,0x00,0x00,0x00,0x07,0x00,0x07,0x00,0x14,0x7F,0x14,0x7F,0x14,0x24,0x2A,0x7F,0x2A,0x12,0x23,0x13,0x08,0x64,0x62,0x36,0x49,0x55,0x22,0x50,0x00,0x05,0x03,0x00,0x00,0x00,0x1C,0x22,0x41,0x00,0x00,0x41,0x22,0x1C,0x00,0x14,0x08,0x3E,0x08,0x14,0x08,0x08,0x3E,0x08,0x08,0x00,0x50,0x30,0x00,0x00,0x08,0x08,0x08,0x08,0x08,0x00,0x60,0x60,0x00,0x00,0x20,0x10,0x08,0x04,0x02,0x3E,0x51,0x49,0x45,0x3E,0x00,0x42,0x7F,0x40,0x00,0x42,0x61,0x51,0x49,0x46,0x21,0x41,0x45,0x4B,0x31,0x18,0x14,0x12,0x7F,0x10,0x27,0x45,0x45,0x45,0x39,0x3C,0x4A,0x49,0x49,0x30,0x01,0x71,0x09,0x05,0x03,0x36,0x49,0x49,0x49,0x36,0x06,0x49,0x49,0x29,0x1E,0x00,0x36,0x36,0x00,0x00,0x00,0x56,0x36,0x00,0x00,0x08,0x14,0x22,0x41,0x00,0x14,0x14,0x14,0x14,0x14,0x00,0x41,0x22,0x14,0x08,0x02,0x01,0x51,0x09,0x06,0x32,0x49,0x79,0x41,0x3E,0x7E,0x11,0x11,0x11,0x7E,0x7F,0x49,0x49,0x49,0x36,0x3E,0x41,0x41,0x41,0x22,0x7F,0x41,0x41,0x22,0x1C,0x7F,0x49,0x49,0x49,0x41,0x7F,0x09,0x09,0x09,0x01,0x3E,0x41,0x49,0x49,0x7A,0x7F,0x08,0x08,0x08,0x7F,0x00,0x41,0x7F,0x41,0x00,0x20,0x40,0x41,0x3F,0x01,0x7F,0x08,0x14,0x22,0x41,0x7F,0x40,0x40,0x40,0x40,0x7F,0x02,0x0C,0x02,0x7F,0x7F,0x04,0x08,0x10,0x7F,0x3E,0x41,0x41,0x41,0x3E,0x7F,0x09,0x09,0x09,0x06,0x3E,0x41,0x51,0x21,0x5E,0x7F,0x09,0x19,0x29,0x46,0x46,0x49,0x49,0x49,0x31,0x01,0x01,0x7F,0x01,0x01,0x3F,0x40,0x40,0x40,0x3F,0x1F,0x20,0x40,0x20,0x1F,0x3F,0x40,0x38,0x40,0x3F,0x63,0x14,0x08,0x14,0x63,0x07,0x08,0x70,0x08,0x07,0x61,0x51,0x49,0x45,0x43,0x00,0x7F,0x41,0x41,0x00,0x02,0x04,0x08,0x10,0x20,0x00,0x41,0x41,0x7F,0x00,0x04,0x02,0x01,0x02,0x04,0x40,0x40,0x40,0x40,0x40,0x00,0x01,0x02,0x04,0x00,0x20,0x54,0x54,0x54,0x78,0x7F,0x48,0x44,0x44,0x38,0x38,0x44,0x44,0x44,0x20,0x38,0x44,0x44,0x48,0x7F,0x38,0x54,0x54,0x54,0x18,0x08,0x7E,0x09,0x01,0x02,0x0C,0x52,0x52,0x52,0x3E,0x7F,0x08,0x04,0x04,0x78,0x00,0x44,0x7D,0x40,0x00,0x20,0x40,0x44,0x3D,0x00,0x7F,0x10,0x28,0x44,0x00,0x00,0x41,0x7F,0x40,0x00,0x7C,0x04,0x18,0x04,0x78,0x7C,0x08,0x04,0x04,0x78,0x38,0x44,0x44,0x44,0x38,0x7C,0x14,0x14,0x14,0x08,0x08,0x14,0x14,0x18,0x7C,0x7C,0x08,0x04,0x04,0x08,0x48,0x54,0x54,0x54,0x20,0x04,0x3F,0x44,0x40,0x20,0x3C,0x40,0x40,0x20,0x7C,0x1C,0x20,0x40,0x20,0x1C,0x3C,0x40,0x30,0x40,0x3C,0x44,0x28,0x10,0x28,0x44,0x0C,0x50,0x50,0x50,0x3C,0x44,0x64,0x54,0x4C,0x44,0x00,0x08,0x36,0x41,0x00,0x00,0x00,0x7F,0x00,0x00,0x00,0x41,0x36,0x08,0x00,0x10,0x08,0x08,0x10,0x08,0x7F,0x7F,0x7F,0x7F,0x7F
};

inline std::string ReadFile(const std::string& p) { std::ifstream f(p); if (!f) throw std::runtime_error("File error"); std::stringstream s; s<<f.rdbuf(); return s.str(); }
inline std::string ReadAll(std::istream& i) { std::stringstream s; s<<i.rdbuf(); return s.str(); }
inline int RunScript(const std::string& src, const std::string& in, std::ostream& o, std::ostream& e) { Lexer l(src); Parser p(l.Scan()); try { Interpreter i(o,e,in); i.Run(p.Parse()); return 0; } catch (std::exception& x) { e<<x.what()<<"\n"; return 1; } }
inline int ParseOnly(const std::string& src, std::ostream& o, std::ostream& e) { Lexer l(src); Parser p(l.Scan()); try { auto s=p.Parse(); o<<"(program"; for(auto& n:s){o<<" ";n->Print(o);} o<<")\n"; return 0; } catch (std::exception& x) { e<<x.what()<<"\n"; return 1; } }

} // namespace
