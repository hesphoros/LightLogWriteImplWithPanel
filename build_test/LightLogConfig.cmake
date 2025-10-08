# LightLogConfig.cmake.in
# CMake configuration file for LightLog library


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was LightLogConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

# 查找依赖
if(WIN32)
    # Windows 平台依赖
else()
    # Linux 平台依赖
    find_dependency(Threads)
endif()

# 包含目标定义
include("${CMAKE_CURRENT_LIST_DIR}/LightLogTargets.cmake")

# 检查组件
check_required_components(LightLog)

# 设置变量
set(LightLog_FOUND TRUE)
set(LightLog_VERSION "1.0.0")
set(LightLog_INCLUDE_DIRS "${PACKAGE_PREFIX_DIR}/include")
set(LightLog_LIBRARIES LightLog::lightlog)

# 向后兼容
set(LIGHTLOG_FOUND ${LightLog_FOUND})
set(LIGHTLOG_VERSION ${LightLog_VERSION})
set(LIGHTLOG_INCLUDE_DIRS ${LightLog_INCLUDE_DIRS})
set(LIGHTLOG_LIBRARIES ${LightLog_LIBRARIES})
