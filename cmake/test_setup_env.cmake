# ============================================================================ #
# 包含一个 test_setup_env 函数，用法：
#
# test_setup_env(<test>)
#
# 给 <test> 添加必要的环境变量。
# ============================================================================ #

set(_dylib_dirs ${Boost_LIBRARY_DIRS})

if(WIN32)
  set(_env_mod "PATH=path_list_prepend:${_dylib_dirs}")
elseif(UNIX)
  set(_env_mod "LD_LIBRARY_PATH=path_list_prepend:${_dylib_dirs}")
else()
  message(WARNING "Unsupported Testing Platform.")
endif()

function(test_setup_env test)
  set_tests_properties(${test} PROPERTIES ENVIRONMENT_MODIFICATION
                                          "${_env_mod}")
endfunction()
