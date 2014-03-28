project(drkonqi)

include (CheckFunctionExists)

find_package(KF5XmlRpcClient CONFIG)

check_function_exists("strsignal" HAVE_STRSIGNAL)
check_function_exists("uname" HAVE_UNAME)

if (NOT DEBUG_PACKAGE_INSTALLER_NAME)
    set (DEBUG_PACKAGE_INSTALLER_NAME "installdbgsymbols.sh")
endif ()

# find_package(KdepimLibs)
set_package_properties(KdepimLibs PROPERTIES DESCRIPTION "The KDE PIM Libraries"
                       URL "http://www.kde.org"
                       TYPE REQUIRED
                       PURPOSE "Required to build Drkonqi"
                      )

configure_file (config-drkonqi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-drkonqi.h )

include_directories(${KDEPIMLIBS_INCLUDE_DIRS})

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1410)

add_subdirectory( data )
add_subdirectory( parser )
add_subdirectory( tests )
if ( WIN32 )
    add_subdirectory( kdbgwin )
endif ()

set(drkonqi_SRCS
    main.cpp
    drkonqidialog.cpp
    statuswidget.cpp
    aboutbugreportingdialog.cpp
    backtraceratingwidget.cpp
    backtracewidget.cpp
    backtracegenerator.cpp
    drkonqi.cpp
    drkonqibackends.cpp
    detachedprocessmonitor.cpp
    debugpackageinstaller.cpp
    systeminformation.cpp
    crashedapplication.cpp
    debugger.cpp
    debuggerlaunchers.cpp
    debuggermanager.cpp
    applicationdetailsexamples.cpp
    gdbhighlighter.cpp
)

if(KF5XmlRpcClient_FOUND)
    set(drkonqi_SRCS
        ${drkonqi_SRCS}
        bugzillalib.cpp
        reportassistantdialog.cpp
        reportassistantpage.cpp
        reportassistantpages_base.cpp
        reportassistantpages_bugzilla.cpp
        reportassistantpages_bugzilla_duplicates.cpp
        reportinterface.cpp
        productmapping.cpp
        parsebugbacktraces.cpp # Requires kxmlrpcclient
        duplicatefinderjob.cpp
    )
    qt5_wrap_ui(drkonqi_SRCS
        ui/assistantpage_introduction.ui
        ui/assistantpage_bugawareness.ui
        ui/assistantpage_conclusions.ui
        ui/assistantpage_conclusions_dialog.ui
        ui/assistantpage_bugzilla_login.ui
        ui/assistantpage_bugzilla_duplicates.ui
        ui/assistantpage_bugzilla_duplicates_dialog.ui
        ui/assistantpage_bugzilla_duplicates_dialog_confirmation.ui
        ui/assistantpage_bugzilla_information.ui
        ui/assistantpage_bugzilla_preview.ui
        ui/assistantpage_bugzilla_send.ui
    )
endif()

qt5_wrap_ui(drkonqi_SRCS
    ui/maindialog.ui
    ui/backtracewidget.ui
)

# if BACKTRACE_PARSER_DEBUG is enabled, it will show both the
# parsed and the unparsed backtrace in the backtrace widget.
# Comment this out for release.
#add_definitions(-DBACKTRACE_PARSER_DEBUG)

add_executable(drkonqi ${drkonqi_SRCS})

target_link_libraries(drkonqi
    KF5::I18n
    KF5::IconThemes
    KF5::WindowSystem
    KF5::Crash
    drkonqi_backtrace_parser
)

if(KF5XmlRpcClient_FOUND)
    target_link_libraries(drkonqi KF5::XmlRpcClient KF5::Wallet KF5::WebKit)
endif()

install(TARGETS drkonqi DESTINATION ${LIBEXEC_INSTALL_DIR})

install(FILES drkonqi-rename-config-section.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR})
