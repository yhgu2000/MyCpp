# 中型 C++ 项目模板

使用方式：

1. 拷贝整个目录

2. 修改项目名

   - [CMakeLists.txt](./CMakeLists.txt)

3. 库

   [lib](./lib) 下提供了一个库模板，使用时注意修改命名空间。

4. 应用

   [app](./app) 中提供了一个应用模板。

   应用源码应该尽可能地少到只有一个文件，其中编写 `main` 函数和命令行解析等。

5. 测试

   本模板使用 Boost.Test 框架，[test](./test) 中提供了一个测试模板。

6. 文档

   本模板使用 Doxygen 生成库的 API 文档。

   在 [doc](./doc) 目录下创建其它 Markdown 文档，然后加到 doxygen 目标中。

## 构建与使用

1. 复制 `env.cmake.example` 为 `env.cmake` ，然后按照你的本地环境修改其中的路径。

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
