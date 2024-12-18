# test 目标名前统一添加 test+ 前缀

if(NOT BUILD_TESTING)
  return()
endif()

# 本项目使用 Boost 的单元测试框架
find_package(Boost COMPONENTS unit_test_framework)
if(NOT Boost_UNIT_TEST_FRAMEWORK_FOUND)
  message(WARNING "Boost.Test 未找到，单元测试已禁用。")
  return()
endif()
if(MSVC AND NOT Boost_USE_STATIC_LIBS)
  target_link_libraries(Boost::unit_test_framework
                        INTERFACE Boost::dynamic_linking)
endif()

#
# 覆盖率测试报告
#
add_code_coverage_all_targets(EXCLUDE test/*)

#
# 测试实用工具
#
add_library(testutil STATIC testutil.hpp testutil.cpp)

target_link_libraries(testutil PUBLIC My Boost::boost
                                      Boost::unit_test_framework)

target_include_directories(testutil PUBLIC .)

#
# 测试目标模板
#
add_executable(test+ test.cpp)

target_link_libraries(test+ PRIVATE testutil)

target_compile_definitions(test+ PRIVATE BOOST_TEST_MODULE=test+)

add_test(NAME test+ COMMAND test+)

target_code_coverage(test+)

# My 库测试
add_subdirectory(My)

# MyHttp 库测试
add_subdirectory(MyHttp)
