#
# 配置生成的时间戳 CMake 脚本
#
configure_file(timestamp.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/timestamp.cmake
               @ONLY)

#
# 就地导出使用方式
#
configure_file(ExportConfig.cmake.in ${CMAKE_PROJECT_NAME}Config.cmake)

export(EXPORT ${EXPORT_TARGETS} NAMESPACE ${CMAKE_PROJECT_NAME}::)

#
# 打包部署使用方式
#
include(CPack)
include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/InstallConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/install/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION cmake)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/install/${PROJECT_NAME}Version.cmake
  COMPATIBILITY AnyNewerVersion)

install(
  EXPORT ${EXPORT_TARGETS}
  DESTINATION cmake # 这里的 DESTINATION 是相对安装目录的路径
  NAMESPACE ${CMAKE_PROJECT_NAME}::)

install(
  FILES depend.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/install/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/install/${PROJECT_NAME}Version.cmake
  DESTINATION cmake)

configure_file(package_readme.md.in package_readme.md)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/package_readme.md
  DESTINATION .
  RENAME README.md)

list(APPEND CPACK_SOURCE_IGNORE_FILES "\\.user") # for Qt Creator
