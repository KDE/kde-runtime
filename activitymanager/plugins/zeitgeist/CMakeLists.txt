project(activitymanager-plugin-zeitgeist)

find_package(QZeitgeist REQUIRED)

set(zeitgeist_SRCS
    ZeitgeistPlugin.cpp
    ../../Plugin.cpp
    ../../SharedInfo.cpp
    )

kde4_add_plugin(
    activitymanager_plugin_zeitgeist
    ${zeitgeist_SRCS}
    )

target_link_libraries(
    activitymanager_plugin_zeitgeist
    ${KDE4_KDECORE_LIBS}
    ${QZEITGEIST_LIBRARY}
    )

include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_CURRENT_BINARY_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${QZEITGEIST_INCLUDE_DIR}
   )

install(TARGETS activitymanager_plugin_zeitgeist DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES activitymanager-plugin-zeitgeist.desktop DESTINATION ${SERVICES_INSTALL_DIR})
