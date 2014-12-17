set(kio_nfs_PART_SRCS kio_nfs.cpp nfsv2.cpp nfsv3.cpp rpc_mnt3_xdr.c rpc_nfs3_prot_xdr.c rpc_mnt2_xdr.c rpc_nfs2_prot_xdr.c )


kde4_add_plugin(kio_nfs ${kio_nfs_PART_SRCS})


target_link_libraries(kio_nfs   ${KDE4_KIO_LIBS})

install(TARGETS kio_nfs  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES nfs.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
