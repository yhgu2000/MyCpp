# library 目标名统一采用 CamelCase



#
# 库目标模板，被设计为可以与 Cmake 项目分离使用，支持直接拷贝出源码集成到上层项目中。
#
file(GLOB Lib_src Lib/*)

add_library(Lib ${Lib_src})

target_include_directories(Lib
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS Lib EXPORT ${EXPORT_TARGETS})
install(DIRECTORY Lib TYPE INCLUDE PATTERN "*.cpp" EXCLUDE)
