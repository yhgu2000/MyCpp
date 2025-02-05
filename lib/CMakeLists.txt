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

target_code_coverage(My AUTO ALL)

#
# “我的 HTTP”客户端/服务器框架。
#
file(GLOB _src MyHttp/*)

add_library(MyHttp ${_src})

find_package(Boost REQUIRED COMPONENTS url)
target_link_libraries(MyHttp PUBLIC My Boost::url)

install(TARGETS MyHttp EXPORT ${EXPORT_TARGETS})
install(
  DIRECTORY MyHttp
  TYPE INCLUDE
  PATTERN "*.cpp" EXCLUDE)

target_code_coverage(MyHttp AUTO ALL)
