#
# 项目配置
#
cmake_minimum_required(VERSION 3.20)

include(env.cmake OPTIONAL RESULT_VARIABLE _env_cmake_found)
if(NOT _env_cmake_found)
  include(environ.cmake)
endif()

# 这里应使用 CamelCase 以贴合 cmake 导出文件的命名风格
project(MyCpp VERSION 0.0.0.0)
include(CTest) # CTest 需要在顶层包含

# 安装导出目标名
set(EXPORT_TARGETS ${PROJECT_NAME}Targets)

# 设置调试配置后缀
set(CMAKE_DEBUG_POSTFIX ".d")

# 项目自己的 CMake 模块包含路径
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(depend) # 引入项目依赖
include(code-coverage) # 覆盖率测试功能

# 项目信息头文件
configure_file(cmake/project.h.in ${PROJECT_BINARY_DIR}/cmake/project.h)
include_directories(${PROJECT_BINARY_DIR}/cmake)

#
# 代码
#
add_subdirectory(lib)

#
# 应用
#
add_subdirectory(app)

#
# 文档
#
add_subdirectory(doc)

#
# 测试
#
add_subdirectory(test)

#
# 脚本
#
add_subdirectory(cmake)
