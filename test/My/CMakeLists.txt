link_libraries(testutil)

#
# 计时器相关测试
#
add_executable(test+My+Timing Timing.cpp)

target_compile_definitions(test+My+Timing PRIVATE BOOST_TEST_MODULE=My+Timing)

add_test(NAME My+Timing COMMAND test+My+Timing)

target_code_coverage(test+My+Timing AUTO ALL)

#
# 自旋锁相关测试
#
add_executable(test+My+SpinMutex SpinMutex.cpp)

target_compile_definitions(test+My+SpinMutex PRIVATE BOOST_TEST_MODULE=My+SpinMutex)

add_test(NAME My+SpinMutex COMMAND test+My+SpinMutex)

target_code_coverage(test+My+SpinMutex AUTO ALL)

#
# 多线程资源池相关测试
#
add_executable(test+My+Pooled Pooled.cpp)

target_compile_definitions(test+My+Pooled PRIVATE BOOST_TEST_MODULE=My+Pooled)

add_test(NAME My+Pooled COMMAND test+My+Pooled)

target_code_coverage(test+My+Pooled AUTO ALL)
