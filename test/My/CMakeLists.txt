#
# 计时器相关测试
#
add_executable(test+My+Timing Timing.cpp)

target_link_libraries(test+My+Timing PRIVATE testutil)

target_compile_definitions(test+My+Timing PRIVATE BOOST_TEST_MODULE=My+Timing)

add_test(NAME My+Timing COMMAND test+My+Timing)
