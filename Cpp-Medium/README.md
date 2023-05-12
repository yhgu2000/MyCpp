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

## 编码规范

见 [doc/coding_conventions.md](doc/coding_conventions.md) 。
