# potatolang

这是一个用 C++ 实现的 Potatolang 解释器与编译器。它不仅支持生成 AST，还支持直接解释执行以及编译为独立二进制文件。

## 构建

### 使用 CMake

```bash
cmake -S . -B build
cmake --build build -j
```

### 直接编译

```bash
clang++ -std=c++17 -o potatolang main.cpp
```

## 使用方法

### 1. 解释执行

直接运行 Potatolang 脚本：

```bash
./potatolang --run script.pt [input_file]
```

- `script.pt`: 源代码文件。
- `input_file`: (可选) 作为标准输入提供给脚本的数据文件。如果不提供，默认为空输入。若要从管道读取标准输入，请使用 `-`。

示例：

```bash
./potatolang --run hw.pt
```

### 2. 编译为独立二进制

将脚本编译为可独立运行的可执行文件（无需依赖 `potatolang` 主程序，但可能依赖标准库文件如 `pio.pt`）：

```bash
./potatolang script.pt --out binary_name
```

示例：

```bash
./potatolang hw.pt --out hw
./hw
```

### 3. 解析并输出 AST

仅进行词法和语法分析，输出 S-expression 形式的抽象语法树（AST）：

```bash
./potatolang script.pt
```

或者从标准输入读取：

```bash
echo 'print "hello";' | ./potatolang
```

## 语言特性

### 关键字

- 变量与函数: `let`, `fun`, `return`
- 控制流: `if`, `else`, `while`
- 模块与IO: `import`, `print`
- 逻辑运算: `true`, `false`, `nil`, `and`, `or`

### 语法示例

```potato
// 导入库
import "pio.pt";

// 定义递归函数
fun fib(n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}

// 变量定义与控制流
let x = 10;
if (x > 5) {
  pio_println("Result: " + fib(x));
}
```

## AST 输出格式

AST 输出形如 S-expression：

```lisp
(program <stmt> <stmt> ...)
```

常见节点：

- `(let name <expr>)`
- `(fun name (params ...) <body>)`
- `(if <cond> <then> <else>)`
- `(while <cond> <body>)`
- `(block <stmt> ...)`
- `(call <callee> <args> ...)`
