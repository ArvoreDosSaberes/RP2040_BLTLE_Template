file(GLOB LIB_SUBDIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*")

foreach(subdir ${LIB_SUBDIRS})
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}" 
     AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt")
    add_subdirectory(${subdir})
  endif()
endforeach()
