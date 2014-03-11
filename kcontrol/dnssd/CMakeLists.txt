


########### next target ###############

set(kcm_kdnssd_PART_SRCS kcmdnssd.cpp kcmdnssd.h )


qt5_wrap_ui(kcm_kdnssd_PART_SRCS configdialog.ui )

add_library(kcm_kdnssd MODULE ${kcm_kdnssd_PART_SRCS})


target_link_libraries(kcm_kdnssd  KF5::DNSSD KF5::KDE4Support )

install(TARGETS kcm_kdnssd  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcm_kdnssd.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
