# library 目标名统一采用 CamelCase



#
# 公共支持库，包含一些常用的功能。
#
file(GLOB _src Common/*)

add_library(Common ${_src})

target_include_directories(Common
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(Common PUBLIC Boost::json)

install(TARGETS Common EXPORT ${EXPORT_TARGETS})
install(DIRECTORY Common TYPE INCLUDE PATTERN "*.cpp" EXCLUDE)
