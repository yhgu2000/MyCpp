# ============================================================================ #
# 包含一个 override_glob 函数，用法：
#
# override_glob(<variable> <origin> <override> <EXPRESSIONS <exprs>...>
# [RECURSE] [ARGUMENTS <args>...])
#
# 用 ${origin}/${exprs} 和 ${override}/${exprs} 进行文件搜索，用后者中相对路径
# 重复的文件替换前者中的文件，然后将绝对路径的文件列表存入 variable 中。
#
# 如果指定了 RECURSE，则使用 file(GLOB_RECURSE 而不是 file(GLOB 。
#
# ARGUMENTS 后的参数将会被原封不动地传递给 file 命令。
# ============================================================================ #

function(override_glob variable origin override)
  cmake_parse_arguments(
    PARSE_ARGV 3 "" "RECURSE" # options
    "" # one_value_keywords
    "EXPRESSIONS;ARGUMENTS" # multi_value_keywords
  )

  if(RECURSE)
    set(glob_command GLOB_RECURSE)
  else()
    set(glob_command GLOB)
  endif()

  # 遍历 EXPRESSIONS，将每个表达式都加上 origin 前缀
  foreach(expression ${_EXPRESSIONS})
    list(APPEND origin_exprs ${origin}/${expression})
  endforeach()

  file(${glob_command} origin_globs RELATIVE ${origin} ${_ARGUMENTS}
       ${origin_exprs})

  # 遍历 EXPRESSIONS，将每个表达式都加上 override 前缀
  foreach(expression ${_EXPRESSIONS})
    list(APPEND override_exprs ${override}/${expression})
  endforeach()

  file(${glob_command} override_globs RELATIVE ${override} ${_ARGUMENTS}
       ${override_exprs})

  # 删掉前者中重复的文件
  list(REMOVE_ITEM origin_globs ${override_globs})

  # 添加对应的前缀，构建最终的绝对路径列表
  foreach(glob ${origin_globs})
    list(APPEND globs ${origin}/${glob})
  endforeach()
  foreach(glob ${override_globs})
    list(APPEND globs ${override}/${glob})
  endforeach()
  set(${variable}
      ${globs}
      PARENT_SCOPE)
endfunction()
