# app 可执行目标名统一采用小写。有些构建系统大小写不敏感，为了避免跨平台兼容性问题
# 不应与库目标重名。



#
# 时间戳目标
#
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/timestamp.h
  ${CMAKE_CURRENT_BINARY_DIR}/timestamp.notexist # 使在每次构建时都执行
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake/timestamp.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(timestamp INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/timestamp.h)

target_include_directories(timestamp INTERFACE ${CMAKE_CURRENT_BINARY_DIR})



#
# 命令行应用模板
#
add_executable(app app.cpp)

target_link_libraries(app PUBLIC timestamp My Boost::program_options)
