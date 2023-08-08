#
# 计时器相关测试
#
add_executable(test_Common_Timing Timing.cpp)

target_link_libraries(test_Common_Timing PRIVATE test_util)

target_compile_definitions(test_Common_Timing PRIVATE BOOST_TEST_MODULE=Timing)

add_test(NAME Common-Timing COMMAND test_Common_Timing)
