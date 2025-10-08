#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "LightLog::lightlog" for configuration ""
set_property(TARGET LightLog::lightlog APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(LightLog::lightlog PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/liblightlog.a"
  )

list(APPEND _cmake_import_check_targets LightLog::lightlog )
list(APPEND _cmake_import_check_files_for_LightLog::lightlog "${_IMPORT_PREFIX}/lib/liblightlog.a" )

# Import target "LightLog::UniConv" for configuration ""
set_property(TARGET LightLog::UniConv APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(LightLog::UniConv PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C;CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libUniConv.a"
  )

list(APPEND _cmake_import_check_targets LightLog::UniConv )
list(APPEND _cmake_import_check_files_for_LightLog::UniConv "${_IMPORT_PREFIX}/lib/libUniConv.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
