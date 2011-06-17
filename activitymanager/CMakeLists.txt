project(ActivityManager)

set (ADDITIONAL_LINK_LIBS)

add_subdirectory(kded)
add_subdirectory(ontologies)

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
    set(QZeitgeist_FOUND TRUE)
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
    EventBackend.cpp
    Event.cpp
    SharedInfo.cpp
    main.cpp
    )

set(activity_manager_backends_SRCS
    NepomukEventBackend.cpp
    NepomukResourceScoreMaintainer.cpp
    NepomukResourceScoreCache.cpp
    ZeitgeistEventBackend.cpp
    SlcEventBackend.cpp
    )

qt4_add_dbus_adaptor(
    activity_manager_SRCS org.kde.ActivityManager.xml
    ActivityManager.h ActivityManager
    )

qt4_add_dbus_adaptor(
    activity_manager_SRCS org.kde.ActivityManager.SLC.xml
    SlcEventBackend.h SlcEventBackend
    )

if(Nepomuk_FOUND)
    soprano_add_ontology(activity_manager_SRCS
        ${CMAKE_SOURCE_DIR}/nepomuk/ontologies/kext.trig
        "KExt" "Nepomuk::Vocabulary" "trig"
        )

    soprano_add_ontology(activity_manager_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/ontologies/krso.trig
        "KRSO" "Nepomuk::Vocabulary" "trig"
        )

    soprano_add_ontology(activity_manager_SRCS
        ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nepomuk/nao.trig
        "NAO" "Nepomuk::Vocabulary" "trig"
        )

    soprano_add_ontology(activity_manager_SRCS
        ${SHAREDDESKTOPONTOLOGIES_ROOT_DIR}/nepomuk/nuao.trig
        "NUAO" "Nepomuk::Vocabulary" "trig"
        )
endif()

kde4_add_executable(activity-manager ${activity_manager_SRCS} ${activity_manager_backends_SRCS})

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

