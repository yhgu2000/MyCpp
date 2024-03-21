# library 目标名统一采用 CamelCase

#
# “我的”公共支持库，包含一些常用的代码。
#
file(GLOB _src My/*)

add_library(My ${_src})

target_include_directories(
  My INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
               $<INSTALL_INTERFACE:include>)

target_link_libraries(My PUBLIC Boost::json Boost::log)

install(TARGETS My EXPORT ${EXPORT_TARGETS})
install(
  DIRECTORY My
  TYPE INCLUDE
  PATTERN "*.cpp" EXCLUDE)

target_code_coverage(My)
