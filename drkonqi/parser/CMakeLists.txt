set(BACKTRACEPARSER_SRCS
    backtraceparser.cpp
    backtraceparsergdb.cpp
    backtraceparserkdbgwin.cpp
    backtraceparsernull.cpp
)

kde4_add_library(drkonqi_backtrace_parser STATIC ${BACKTRACEPARSER_SRCS})
target_link_libraries(drkonqi_backtrace_parser ${KDE4_KDECORE_LIBS})
