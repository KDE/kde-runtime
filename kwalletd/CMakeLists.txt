project(kwalletd)

########### find needed packages ######
find_package(Gpgme)  # Called by FindQGpgme, but since we call some gpgme
                     # functions ourselves we need to link against it directly.
find_package(QGpgme) # provided by kdepimlibs

if (NOT QGPGME_FOUND)
find_package(Gpgmepp) # provided by gpgme 1.7
endif (NOT QGPGME_FOUND)

if ((GPGME_FOUND AND QGPGME_FOUND) OR Gpgmepp_FOUND)
    add_definitions(-DHAVE_QGPGME)
    include_directories(${GPGME_INCLUDES})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
endif((GPGME_FOUND AND QGPGME_FOUND) OR Gpgmepp_FOUND)
if (GPGME_FOUND AND QGPGME_FOUND)
    include_directories(${QGPGME_INCLUDE_DIR})
endif (GPGME_FOUND AND QGPGME_FOUND)
if (Gpgmepp_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif (Gpgmepp_FOUND)

########### build backends #########
add_subdirectory(backend)

########### kwalletd ###############

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backend)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=285)

set(kwalletd_KDEINIT_SRCS
   main.cpp
   kbetterthankdialog.cpp
   kwalletd.cpp
   kwalletwizard.cpp
   ktimeout.cpp
   kwalletsessionstore.cpp
)

kde4_add_ui_files(kwalletd_KDEINIT_SRCS
   kbetterthankdialogbase.ui
   kwalletwizardpageexplanation.ui
   kwalletwizardpageintro.ui
   kwalletwizardpageoptions.ui
   kwalletwizardpagepassword.ui
)

if ((GPGME_FOUND AND QGPGME_FOUND) OR Gpgmepp_FOUND)
    set(kwalletd_KDEINIT_SRCS
        ${kwalletd_KDEINIT_SRCS}
        knewwalletdialog.cpp
    )
    kde4_add_ui_files(kwalletd_KDEINIT_SRCS
        kwalletwizardpagepasswordgpg.ui
        kwalletwizardpagegpgkey.ui
        knewwalletdialogintro.ui
        knewwalletdialoggpg.ui
    )
endif((GPGME_FOUND AND QGPGME_FOUND) OR Gpgmepp_FOUND)

find_file(kwallet_xml org.kde.KWallet.xml HINTS ${KDE4_DBUS_INTERFACES_DIR} )

qt4_add_dbus_adaptor( kwalletd_KDEINIT_SRCS ${kwallet_xml} kwalletd.h KWalletD )

kde4_add_kdeinit_executable( kwalletd NOGUI ${kwalletd_KDEINIT_SRCS} )

target_link_libraries(kdeinit_kwalletd ${KDE4_KDEUI_LIBS} kwalletbackend )
if(GPGME_FOUND AND QGPGME_FOUND)
target_link_libraries(kdeinit_kwalletd ${QGPGME_LIBRARIES} )
else(GPGME_FOUND AND QGPGME_FOUND)
if(Gpgmepp_FOUND)
target_link_libraries(kdeinit_kwalletd Gpgmepp)
endif(Gpgmepp_FOUND)
endif(GPGME_FOUND AND QGPGME_FOUND)

install(TARGETS kdeinit_kwalletd  ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries(kwalletd kdeinit_kwalletd)
install(TARGETS kwalletd  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES kwalletd.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kwalletd.notifyrc DESTINATION  ${DATA_INSTALL_DIR}/kwalletd )
install( FILES kwallet-4.13.upd DESTINATION ${DATA_INSTALL_DIR}/kconf_update)

add_subdirectory(tests)
add_subdirectory(autotests)
