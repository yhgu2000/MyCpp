find_package(
  Doxygen
  COMPONENTS dot
  OPTIONAL_COMPONENTS mscgen dia)

if(NOT DOXYGEN_FOUND)
  message(WARNING "Doxygen 未找到，文档生成已禁用。")
  return()
endif()

# CMake 依赖图
add_custom_command(
  OUTPUT targets.dot targets.png
  COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR} --graphviz=targets.dot
          ${PROJECT_SOURCE_DIR}
  COMMAND Doxygen::dot -Tpng targets.dot -o targets.png
  DEPENDS ${PROJECT_BINARY_DIR}/CMakeFiles/TargetDirectories.txt
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

#
# Doxygen 库 API 文档
#
set(DOXYGEN_IMAGE_PATH . ${CMAKE_CURRENT_BINARY_DIR})
set(DOXYGEN_USE_MATHJAX YES)
set(DOXYGEN_CREATE_SUBDIRS YES)
set(DOXYGEN_GENERATE_TREEVIEW YES)

doxygen_add_docs(
  doxygen
  # 主页
  mainpage.md
  ${CMAKE_CURRENT_BINARY_DIR}/targets.png
  # 提取代码中的注释
  ../lib)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION doc)
