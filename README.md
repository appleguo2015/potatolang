# potatolang

这是一个用 C++ 实现的 Potatolang 解释器与编译器。它不仅支持生成 AST，还支持直接解释执行以及编译为独立二进制文件。最新版本集成了 SDL2，支持图形化界面开发。

## 构建

### 前置依赖

需要安装 SDL2 开发库：

**macOS (Homebrew):**
```bash
brew install sdl2 pkg-config
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libsdl2-dev
```

### 编译 Potatolang

```bash
clang++ -std=c++17 main.cpp -o potatolang $(pkg-config --cflags --libs sdl2)
```

### 编译 Tomato Editor

```bash
clang++ -std=c++17 tomato/main.cpp -o tomato/tomato $(pkg-config --cflags --libs sdl2)
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

将脚本编译为可独立运行的可执行文件（自动链接 SDL2）：

```bash
./potatolang script.pt --out binary_name
```

示例：

```bash
# 编译并运行贪吃蛇游戏
./potatolang snake.pt --out snake
./snake
```

### 3. 解析并输出 AST

仅进行词法和语法分析，输出 S-expression 形式的抽象语法树（AST）：

```bash
./potatolang script.pt
```

### 4. 原生编辑器 (Native Editor)

启动独立的 Tomato C++ 原生代码编辑器：

```bash
./tomato/tomato script.pt
```

此编辑器支持：
- **语法高亮**：支持关键字、字符串、数字和注释的高亮显示。
- **行号显示**：左侧显示行号。
- **输出面板**：直接在编辑器内查看运行结果。

快捷键：
- **F2**: 保存文件。
- **F5**: 保存并运行当前脚本（显示输出面板）。
- **Esc**: 关闭输出面板或退出编辑器。

## 语言特性

### 关键字
- 变量与函数: `let`, `fun`, `return`
- 控制流: `if`, `else`, `while`
- 模块与IO: `import`, `print`
- 逻辑运算: `true`, `false`, `nil`, `and`, `or`

### 内置函数

#### 图形界面 (SDL2)
- `graphics_init(width, height, title)`: 初始化窗口，返回布尔值。
- `graphics_clear()`: 清空屏幕。
- `graphics_color(r, g, b)`: 设置绘图颜色 (0-255)。
- `graphics_rect(x, y, w, h)`: 绘制实心矩形。
- `graphics_present()`: 更新屏幕显示。
- `graphics_poll()`: 获取按键事件（返回 "Up", "Down", "Left", "Right", "W", "A", "S", "D", "Q", "Escape", "quit" 或 nil）。

#### 系统与工具
- `sleep(ms)`: 暂停指定毫秒数。
- `random()`: 返回 0.0 到 1.0 之间的随机数。
- `time()`: 返回当前时间戳（秒）。
- `read_line()`: 从标准输入读取一行。
- `int(value)`: 将数值或字符串转换为整数。

#### 列表操作
- `list()`: 创建空列表。
- `push(lst, val)`: 向列表追加元素。
- `get(lst, idx)`: 获取列表元素。
- `set(lst, idx, val)`: 修改列表元素。
- `len(lst)`: 获取列表长度。
- `remove_at(lst, idx)`: 删除指定索引的元素。

#### 字符串操作
- 支持字符串拼接 `+` 和重复 `*` (例如 `"a" * 3` 得到 `"aaa"`)。
- `to_string(val)`: 将值转换为字符串。

### 标准库模块

#### 文件 IO (potato_file)
需要导入模块: `import "potato_file.pt";`

- `file_exists(path)`: 检查文件是否存在。
- `file_read(path)`: 读取整个文件内容。
- `file_write(path, content)`: 写入内容到文件（覆盖）。
- `file_append(path, content)`: 追加内容到文件。

### 示例项目

#### 1. 贪吃蛇 (snake.pt)
项目中包含一个完整的图形化贪吃蛇游戏示例。

```bash
./potatolang --run snake.pt
```

#### 2. Tomato 编辑器 (tomato/)
一个轻量级的 TUI 代码编辑器，支持通过 `.potato` 文件配置语法高亮。

**运行方法：**
```bash
./potatolang --run tomato/tomato.pt
```

**目录结构：**
- `tomato/tomato.pt`: 主入口
- `tomato/editor.pt`: 编辑器核心逻辑
- `tomato/highlighter.pt`: 语法高亮引擎
- `tomato/syntax_loader.pt`: `.potato` 配置文件加载器
- `tomato/potatolang.potato`: Potatolang 语法定义示例

**配置语法高亮 (.potato):**
```text
keywords:let,fun,if,else,while
comment://
string:"
```
