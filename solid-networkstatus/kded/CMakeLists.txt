########### next target ###############

set(kded_networkstatus_PART_SRCS
   networkstatus.cpp
   network.cpp
   systemstatusinterface.cpp
   wicdstatus.cpp
   wicdcustomtypes.cpp
)

find_package(NetworkManager 0.7.0)
set_package_properties(NetworkManager PROPERTIES DESCRIPTION "The NetworkManager headers"
                       URL "http://projects.gnome.org/NetworkManager"
                       TYPE OPTIONAL
                       PURPOSE "Needed for kded's networkstatus module"
                      )

if(NETWORKMANAGER_FOUND)
    if(NM_0_7)
        message(STATUS "Adding in-process NetworkManager service-wart to kded module")
        set(kded_networkstatus_PART_SRCS ${kded_networkstatus_PART_SRCS} networkmanagerstatus.cpp)
        include_directories(${NETWORKMANAGER_INCLUDE_DIRS})
        add_definitions(-DNM_BACKEND_ENABLED)
    endif()
endif()


# FIXME: Re-enable the above when:
# * QNTrack has been ported to Qt5
# * cmake/modules/FindQNtrack.cmake has been adapted to the Qt5 dependency

#find_package(QNtrack)
#set_package_properties(QNtrack PROPERTIES DESCRIPTION "Network status tracking library"
#                       URL "http://launchpad.net/ntrack"
#                       TYPE OPTIONAL
#                       PURPOSE "Provides data input for Solid network status"
#                      )
set(QNTRACK_FOUND FALSE) # Forced to false, see above FIXME

if(QNTRACK_FOUND)
    message(STATUS "Found QNtrack, will use it as a data input for Solid network status")
    set(kded_networkstatus_PART_SRCS ${kded_networkstatus_PART_SRCS} ntracknetworkstate.cpp)
    add_definitions(-DHAVE_QNTRACK)
else()
    message(STATUS "QNtrack not found.  Can be used as a data input to Solid network status.  See https://launchpad.net/ntrack")
endif()

include_directories(${QNTRACK_INCLUDE_DIR})
qt5_add_dbus_adaptor(kded_networkstatus_PART_SRCS ../org.kde.Solid.Networking.Service.xml networkstatus.h NetworkStatusModule)

# we need one of the interfaces installed by kdelibs:
qt5_add_dbus_adaptor(kded_networkstatus_PART_SRCS
        ${SOLID_DBUS_INTERFACES_DIR}/kf5_org.kde.Solid.Networking.Client.xml
        networkstatus.h NetworkStatusModule)


add_library(kded_networkstatus MODULE ${kded_networkstatus_PART_SRCS})

target_link_libraries(kded_networkstatus KF5::DBusAddons KF5::Solid KF5::KDE4Support)

if(QNTRACK_FOUND)
   target_link_libraries(kded_networkstatus ${QNTRACK_LIBRARIES})
endif()

install(TARGETS kded_networkstatus DESTINATION ${PLUGIN_INSTALL_DIR})

install( FILES networkstatus.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

