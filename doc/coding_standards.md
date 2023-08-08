# 编码规范

原则：**遵守规范，但不要死板**

制定规范的根本目的是让代码易读，如果规范本身和这一目的冲突，则应毫不犹豫地突破规范。

> 当然，本规范已经经历了长期检验，一般情况下不会出现这种情况。

## 使用 clang-format 格式化代码

除了下面的标识符命名法之外，其余遵循 Mozilla 风格，项目已配置好 `.clang-format` 文件。

## 标识符命名法

> 项目已配置好 `.clang-tidy` 进行命名检查。

1. 设计思想：向标识符中加入作用域信息。

   原因是：

   - 可以起到提醒作用
     > 不同作用域的标识符行为差别非常大，而相比于类型信息，作用域信息很难由 IDE 方便地显示。
   - 可以最大程度避免名称冲突
     > 降低命名困难症的发作频率

2. 几种命名风格

   - `lower_case`
   - `UPPER_CASE`
   - `camelBack`
   - `CamelCase`
   - `camel_Snake_Back`
   - `Camel_Snake_Case`
   - `aNy_CasE`

3. 宏

   使用 `UPPER_CASE`，并且加上项目名前缀以避免和其它项目的头文件冲突。

   例外：

   - `#define self (*this)`

     只可用于源文件，禁止写入头文件

4. 命名空间

   使用 `Camel_Snake_Case`，并在使用处赋以别名以免代码过于冗余。

5. 类型

   使用 `CamelCase`，包括 `class`，`struct`，`union`，`enum`，`typedef`，`using` 以及模板的类型参数等一切性质为类型的标识符。

6. 函数

   一般情况下使用 `lower_case`，包括全局函数、类静态函数、类方法等。

   例外情况：

   - 类成员的 `get`、`set` 方法为成员名加上 `get_`、`set_` 前缀
   - 对于 Qt 的信号和槽，分别为 `s_`、`w_` 加上具体描述，例如：

     ```cpp
     void s_someButton_clickedTwice();
     void w_someButton_clickedTwice();
     ```

7. 变量

   使用 `camelBack`，包括类成员、局部变量、函数参数等。如果前有作用域标识，则原名称应改为 `CamelCase`，使得整体仍为 `camelBack`，如： `var` 变成 `kVar`，`str` 变成 `gStr`。

   作用域标识：

   - 常量前加 `k`

     包括 `const`、`constexpr`、`enum`枚举值等一切性质为常量的标识符。

     > 注意 `const char* _` 是变量，`const char* const _` 才是常量。

   - 全局变量前加 `g`

     包括全局变量、类静态变量。全局线程本地变量前加 `gt`。

   - 局部静态变量前加 `s`

     局部线程本地变量前加 `st`。

   - 类成员变量前加 `m`

8. 让变量名的长度适应作用域影响范围

   可以使用 `a`、`b`、`c` 这种非常简单随意的名称，但只限于局部的临时变量等极小的范围。

   常见的作用域影响范围从大到小排序为：

   1. 宏
   2. 命名空间
   3. 类名、全局变量、全局函数等
   4. 类的静态成员和静态方法
   5. 类方法和类成员
   6. 函数参数
   7. 局部静态变量和局部线程本地变量
   8. 局部变量

   鼓励使用缩写，采用首字母拼接成的缩写在和其它单词拼接成标识符时将第一个字母后的首字母改为小写，如 `Curiously Recurring Template Pattern Test` 保留 `Test` 的缩写为 `CrtpTest`。**如果使用自造的缩写词，则在文档或第一次出现的地方注释原本的名字。**

   > C++ 程序员不写 Java 那种 `thisIsaTemporaryVariable` 的冗长变量名！

# 其它

1. C++ 头文件使用 `.hpp` 后缀，C 及 C/C++ 通用的头文件使用 `.h` 后缀。

2. 使用 `#pragma once` 代替头文件卫士宏。
