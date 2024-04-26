# 中型 C++ 项目模板

使用方式：

1. 拷贝整个目录

2. 修改项目名

   - [CMakeLists.txt](./CMakeLists.txt)

3. 库

   在 [lib](./lib) 下创建新的库。模板已经包含了一个 `Common` 库，提供了一些常用功能，如异常类、性能计时器等。

4. 应用

   [app](./app) 中提供了一个命令行应用模板。

   应用源码应该尽可能地少到只有一个文件，其中只编写 `main` 函数和命令行解析等驱动代码。

5. 测试

   本模板使用 Boost.Test 框架，[test](./test) 中提供了一个测试模板。

6. 文档

   本模板使用 Doxygen 生成库的 API 文档。

   在 [doc](./doc) 目录下创建其它 Markdown 文档，然后加到 doxygen 目标中。

## 构建与使用

1. 复制 `environ.cmake` 为 `env.cmake` ，然后按照你的本地环境修改其中的路径。

2. 运行 CMake 配置过程：

   ```bash
   mkdir build
   cd build
   cmake ..
   ```

   > 默认构建的是 Debug 版本，如果想构建其他版本（如 Release），请设置 [`CMAKE_BUILD_TYPE` 变量](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)。

3. 构建所有目标：

   ```bash
   cmake --build .
   ```

   > 如果想构建项目的单元测试，需要安装 [Boost.Test](https://www.boost.org) 库。

   > 如果想构建项目的文档，需要安装 [Doxygen](https://www.doxygen.nl) 和 [Graphviz](https://graphviz.org)。

4. 打包发布：

   项目使用 CPack，可以生成各种格式的包文件，详请参阅[官方文档](https://cmake.org/cmake/help/latest/manual/cpack.1.html)。

   打包构建产物（可用于二次开发）：

   ```bash
   cpack -G ZIP
   ```

   打包源码：

   ```bash
   cpack --config CPackSourceConfig.cmake
   ```

## 编码规范

见 [doc/coding_standards.md](doc/coding_standards.md) 。

## 其它

- 关于导出 SDK 给其它项目使用

  目前已知没有容易的方法能解决导出 SDK 时的命名空间冲突问题（比如 `Common` 模块）。同时，考虑到大部分中小型项目并不会发展到产生这个需求的规模，因此本模板不提供 SDK 导出的完整支持。

  如果想导出 SDK 给其它项目使用，不可避免地，需要对项目结构作出一定调整，包括并不限于以下工作：

  - 重构项目的命名空间；
  - 重新编写 CMake 安装规则（`install`命令）；
  - ...

# 第三方许可

- [cmake-scripts](https://github.com/StableCoder/cmake-scripts/blob/main/LICENSE)

  使用了其中的覆盖率测试 CMake 脚本。
