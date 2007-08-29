macro_optional_find_package(Xine)
if(XINE_FOUND)
  macro_optional_add_subdirectory(xine)
endif(XINE_FOUND)
