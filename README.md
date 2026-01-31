# potatolang

这是一个用 C++ 实现的最小可用 potatolang：包含词法分析 + 递归下降语法分析，读取源码后输出 AST（S-expression 形式）。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

从文件读取：

```bash
./build/potatolang path/to/file.pt
```

从标准输入读取：

```bash
printf 'let x = 1 + 2 * 3;\nprint x == 7;\n' | ./build/potatolang
```

输出是 AST，例如：

```lisp
(program (let x (+ 1 (* 2 3))) (print (== x 7)))
```

## 词法（Token）

### 关键字

- `let`
- `print`

### 字面量与标识符

- 数字：`123`、`3.14`
- 字符串：`"hello"`，支持转义：`\\n`、`\\t`、`\\"`、`\\\\`
- 标识符：以字母或 `_` 开头，后续可包含数字，例如：`abc`、`_tmp1`

### 运算符与符号

- 括号：`(` `)`
- 语句结尾：`;`
- 运算符：`+ - * / ! = < >`
- 双字符运算符：`== != <= >=`

### 空白与注释

- 空白：空格、制表符、换行会被忽略（用于分隔 token）
- 行注释：`//` 到行尾

## 语法（Grammar）

语句以分号 `;` 结束。

EBNF（接近实现的形式）：

```ebnf
program      -> stmt* EOF ;

stmt         -> let_stmt
             | print_stmt
             | expr ";" ;

let_stmt     -> "let" IDENT "=" expr ";" ;
print_stmt   -> "print" expr ";" ;

expr         -> equality ;
equality     -> comparison ( ( "==" | "!=" ) comparison )* ;
comparison   -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term         -> factor ( ( "+" | "-" ) factor )* ;
factor       -> unary ( ( "*" | "/" ) unary )* ;
unary        -> ( "!" | "-" ) unary
             | primary ;
primary      -> NUMBER
             | STRING
             | IDENT
             | "(" expr ")" ;
```

## 优先级与结合性

从高到低（越靠上优先级越高）：

1. 括号：`(expr)`
2. 一元：`!`、`-`
3. 乘除：`*`、`/`
4. 加减：`+`、`-`
5. 比较：`>`、`>=`、`<`、`<=`
6. 相等：`==`、`!=`

二元运算都是左结合，例如：`1 - 2 - 3` 解析为 `(- (- 1 2) 3)`。

## AST 输出格式

程序输出形如：

```lisp
(program <stmt> <stmt> ...)
```

常见节点：

- `let`：`(let name <expr>)`
- `print`：`(print <expr>)`
- 表达式语句：`(expr <expr>)`
- 二元：`(<op> <left> <right>)`，例如：`(+ 1 2)`、`(== x 7)`
- 一元：`(<op> <right>)`，例如：`(- 1)`、`(! x)`
- 分组：`(group <expr>)`

## 示例

```potato
// 变量声明
let x = 1 + 2 * 3;

// 打印表达式
print x == 7;

// 表达式语句
(1 + 2) * 3;
```

可能输出：

```lisp
(program (let x (+ 1 (* 2 3))) (print (== x 7)) (expr (* (group (+ 1 2)) 3)))
```

## 错误信息

- 词法错误：`Lex error at line:column: <reason-or-char>`
- 语法错误：`Parse error at line:column: <message>`

