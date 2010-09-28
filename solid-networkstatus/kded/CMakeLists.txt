########### next target ###############

set(kded_networkstatus_PART_SRCS
   networkstatus.cpp
   network.cpp
   systemstatusinterface.cpp
)

macro_optional_find_package(NetworkManager)

if(NETWORKMANAGER_FOUND)
    macro_ensure_version("0.7.0" ${NETWORKMANAGER_VERSION} NM_0_7)
    if(NM_0_7)
        MESSAGE(STATUS "Adding in-process NetworkManager service-wart to kded module")
        set(kded_networkstatus_PART_SRCS ${kded_networkstatus_PART_SRCS} networkmanagerstatus.cpp)
        include_directories(${NETWORKMANAGER_INCLUDE_DIRS})
        add_definitions(-DNM_BACKEND_ENABLED)
    endif(NM_0_7)
endif(NETWORKMANAGER_FOUND)

find_package(QNtrack)

if(QNTRACK_FOUND)
    MESSAGE(STATUS "Found QNtrack, will use it as a data input for Solid network status")
    set(kded_networkstatus_PART_SRCS ${kded_networkstatus_PART_SRCS} ntracknetworkstate.cpp)
    add_definitions(-DHAVE_QNTRACK)
else(QNTRACK_FOUND)
    MESSAGE(STATUS "QNtrack not found.  Can be used as a data input to Solid network status.  See https://launchpad.net/ntrack")
endif(QNTRACK_FOUND)

macro_log_feature(QNTRACK_FOUND "QNtrack" "Network status tracking library" "http://launchpad.net/ntrack" FALSE "" "Provides data input for Solid network status")

include_directories(${QNTRACK_INCLUDE_DIR})

qt4_add_dbus_adaptor(kded_networkstatus_PART_SRCS ../org.kde.Solid.Networking.Service.xml
        networkstatus.h NetworkStatusModule)

# we need one of the interfaces installed by kdelibs:
qt4_add_dbus_adaptor(kded_networkstatus_PART_SRCS
        ${KDE4_DBUS_INTERFACES_DIR}/org.kde.Solid.Networking.Client.xml
        networkstatus.h NetworkStatusModule)


kde4_add_plugin(kded_networkstatus ${kded_networkstatus_PART_SRCS})

if(QNTRACK_FOUND)
   target_link_libraries(kded_networkstatus ${KDE4_KDECORE_LIBS} ${QNTRACK_LIBRARIES})
else(QNTRACK_FOUND)
   target_link_libraries(kded_networkstatus ${KDE4_KDECORE_LIBS})
endif(QNTRACK_FOUND)

install(TARGETS kded_networkstatus DESTINATION ${PLUGIN_INSTALL_DIR})

install( FILES networkstatus.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)

