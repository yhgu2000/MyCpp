# app 可执行目标名统一采用小写，如出现和库目标重名的情况，建议在之后添加 -app 后缀，
# 然后设置目标的 OUTPUT_NAME 修正生成的文件名。



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
# 应用目标模板
#
add_executable(app app.cpp)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS program_options)

target_link_libraries(app PUBLIC timestamp Lib Boost::program_options)
