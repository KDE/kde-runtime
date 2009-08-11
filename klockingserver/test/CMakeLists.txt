set(lockingtest_SRCS lockingtest.cpp kipclock.cpp )

kde4_add_executable(lockingtest NOGUI ${lockingtest_SRCS})

target_link_libraries(lockingtest ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})
