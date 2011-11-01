project(ActivityManager)

set (ADDITIONAL_LINK_LIBS)

add_subdirectory(kded)

# Checking for Nepomuk
macro_optional_find_package(Nepomuk)
macro_log_feature(
        Nepomuk_FOUND
        "Nepomuk" "Nepomuk" "http://www.kde.org" FALSE ""
        "STRONGLY_RECOMMENDED: Nepomuk is needed for some activity-related info")

if(Nepomuk_FOUND)
    set(HAVE_NEPOMUK 1)
    set(ADDITIONAL_LINK_LIBS
        ${ADDITIONAL_LINK_LIBS}
        ${NEPOMUK_LIBRARIES}
        ${NEPOMUK_QUERY_LIBRARIES}
        ${SOPRANO_LIBRARIES}
    )
endif(Nepomuk_FOUND)

# Checking for QtZeitgeist
macro_optional_find_package(QZeitgeist)

if (QZEITGEIST_INCLUDE_DIR)
    if (QZEITGEIST_VERSION VERSION_LESS 0.8.0)
        set(QZeitgeist_FOUND TRUE)
    else (QZEITGEIST_VERSION VERSION_LESS 0.8.0)
        message(STATUS "QZeitgeist 0.8.0 and greater are not supported")
        set(QZeitgeist_FOUND FALSE)
    endif (QZEITGEIST_VERSION VERSION_LESS 0.8.0)
endif(QZEITGEIST_INCLUDE_DIR)

macro_log_feature(
        QZeitgeist_FOUND
        "QtZeitgeist" "Qt bindings for Zeitgeist" "http://gitorious.org/kde-zeitgeist/libqzeitgeist/" FALSE ""
        "RECOMMENDED: Zeitgeist and QtZeitgeist is needed for resource tracking")
if (QZeitgeist_FOUND)
    set(HAVE_QZEITGEIST 1)
    include_directories(${QZEITGEIST_INCLUDE_DIR})
    set(ADDITIONAL_LINK_LIBS
        ${ADDITIONAL_LINK_LIBS}
        ${QZEITGEIST_LIBRARY}
    )
endif(QZeitgeist_FOUND)

# config file
configure_file(config-features.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-features.h )

# Standard stuff
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${KDE4_INCLUDES}
    )

set(activity_manager_SRCS
    ActivityManager.cpp
    EventProcessor.cpp
    ZeitgeistEventBackend.cpp
    NepomukEventBackend.cpp
    EventBackend.cpp
    Event.cpp
    main.cpp
    )

qt4_add_dbus_adaptor(
    activity_manager_SRCS org.kde.ActivityManager.xml
    ActivityManager.h ActivityManager
    )

if(Nepomuk_FOUND)
    soprano_add_ontology(activity_manager_SRCS ${CMAKE_SOURCE_DIR}/nepomuk/ontologies/kext.trig "KExt" "Nepomuk::Vocabulary" "trig")
endif()

kde4_add_executable(activity-manager ${activity_manager_SRCS})

target_link_libraries(
    activity-manager
    ${KDE4_KIO_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${ADDITIONAL_LINK_LIBS}
    )

set_target_properties(activity-manager PROPERTIES OUTPUT_NAME kactivitymanagerd)

########### install application ###############

install(
    FILES kactivitymanagerd.desktop DESTINATION ${SERVICES_INSTALL_DIR}
    )

install(
    TARGETS activity-manager ${INSTALL_TARGETS_DEFAULT_ARGS}
    )

