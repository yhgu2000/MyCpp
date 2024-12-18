link_libraries(testutil MyHttp)

#
# 基础 HelloWorld 测试
#
add_executable(test+MyHttp+HelloWorld HelloWorld.cpp)

target_compile_definitions(test+MyHttp+HelloWorld
  PRIVATE
    BOOST_TEST_MODULE=MyHttp+HelloWorld)

add_test(NAME MyHttp+HelloWorld COMMAND test+MyHttp+HelloWorld)

target_code_coverage(test+MyHttp+HelloWorld AUTO ALL)
