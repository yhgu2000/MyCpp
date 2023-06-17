#
# 这个文件中用于在导出时向上层依赖的项目传递下层依赖
#



# 语言标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)



# MSVC 配置
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")



# 在一个项目里依赖最好不要混用 Boost 动/静态链接
option(Boost_USE_STATIC_LIBS "是否将 Boost 静态链接到目标中。" OFF)
find_package(Boost REQUIRED COMPONENTS program_options json)
if (NOT Boost_USE_STATIC_LIBS)
  target_link_libraries(Boost::boost INTERFACE Boost::dynamic_linking)
  target_link_libraries(Boost::program_options INTERFACE Boost::dynamic_linking)
  target_link_libraries(Boost::json INTERFACE Boost::dynamic_linking)
endif()
