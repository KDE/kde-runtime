project(nepomukontologies)

configure_file(nrl.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nrl.desktop)
configure_file(nao.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nao.desktop)
configure_file(nie.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nie.desktop)
configure_file(nfo.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nfo.desktop)

configure_file(ncal.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/ncal.desktop)
configure_file(nco.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nco.desktop)
configure_file(nmo.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nmo.desktop)
configure_file(nid3.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nid3.desktop)
configure_file(nexif.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nexif.desktop)

configure_file(rdf.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/rdf.desktop)
configure_file(rdfs.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/rdfs.desktop)
configure_file(dcq.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/dcq.desktop)
configure_file(dctype.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/dctype.desktop)
configure_file(dces.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/dces.desktop)
configure_file(xesam.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/xesam.desktop)
configure_file(pimo.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/pimo.desktop)
#configure_file(nmisc.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nmisc.desktop)

install(FILES
  nrl.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nrl.desktop
  nao.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nao.desktop
  nie.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nie.desktop
  nfo.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nfo.desktop

  ncal.trig
  ${CMAKE_CURRENT_BINARY_DIR}/ncal.desktop
  nco.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nco.desktop
  nmo.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nmo.desktop
  nid3.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nid3.desktop
  nexif.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nexif.desktop

  pimo.trig
  ${CMAKE_CURRENT_BINARY_DIR}/pimo.desktop
#  nmisc.trig
#  ${CMAKE_CURRENT_BINARY_DIR}/nmisc.desktop
  rdf.rdfs
  ${CMAKE_CURRENT_BINARY_DIR}/rdf.desktop
  rdfs.rdfs
  ${CMAKE_CURRENT_BINARY_DIR}/rdfs.desktop
  ${CMAKE_CURRENT_BINARY_DIR}/dces.desktop
  dces.rdf
  ${CMAKE_CURRENT_BINARY_DIR}/dcq.desktop
  dcq.rdf
  ${CMAKE_CURRENT_BINARY_DIR}/dctype.desktop
  dctype.rdf
  ${CMAKE_CURRENT_BINARY_DIR}/xesam.desktop
  xesam.rdfs
  DESTINATION
  ${DATA_INSTALL_DIR}/nepomuk/ontologies
)
