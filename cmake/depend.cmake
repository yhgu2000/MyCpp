#
# 这个文件中用于在导出时向上层依赖的项目传递下层依赖
#



# 语言标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)



# 设置调试配置后缀
set(CMAKE_DEBUG_POSTFIX ".d")
# 设置可执行和动态库的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")



# MSVC 配置
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")



# 在一个项目里依赖最好不要混用 Boost 动/静态链接
option(Boost_USE_STATIC_LIBS "是否静态链接 Boost 库。" OFF)
# set(Boost_DEBUG ON)
set(_boost_components program_options json coroutine log)
find_package(Boost REQUIRED COMPONENTS ${_boost_components})
if (MSVC AND NOT Boost_USE_STATIC_LIBS)
  foreach(_boost_component IN LISTS _boost_components)
    target_link_libraries(Boost::${_boost_component}
      INTERFACE Boost::dynamic_linking)
  endforeach()
endif()
