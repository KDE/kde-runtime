install( FILES kcm_kemail.desktop kcm_browser.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kcm_componentchooser )
if(NOT WIN32)
  install( FILES kcm_terminal.desktop DESTINATION  ${DATA_INSTALL_DIR}/kcm_componentchooser)
endif(NOT WIN32)
