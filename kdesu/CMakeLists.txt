check_include_files(sys/wait.h HAVE_SYS_WAIT_H)

if (UNIX AND NOT APPLE)
    add_subdirectory( kdesud )
endif()
