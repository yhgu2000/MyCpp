link_libraries(testutil)

#
# 计时器相关测试
#
add_executable(test+My+Timing Timing.cpp)

target_compile_definitions(test+My+Timing PRIVATE BOOST_TEST_MODULE=My+Timing)

add_test(NAME My+Timing COMMAND test+My+Timing)

target_code_coverage(test+My+Timing)
