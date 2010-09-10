set(kaudiodevicelist_LIB_SRCS
    audiodevice.cpp
    audiodeviceenumerator.cpp
    hardwaredatabase.cpp
)

set(KDE4_KAUDIODEVICELIST_LIBS
    ${KDE4_KDECORE_LIBS}
    ${KDE4_SOLID_LIBS}
)

if(ALSA_FOUND)
    qt4_add_resources(kaudiodevicelist_LIB_SRCS resources.qrc)
    set(KDE4_KAUDIODEVICELIST_LIBS
        ${KDE4_KAUDIODEVICELIST_LIBS}
        ${ASOUND_LIBRARY}
    )
endif(ALSA_FOUND)

kde4_add_library(kaudiodevicelist SHARED ${kaudiodevicelist_LIB_SRCS})
target_link_libraries(kaudiodevicelist ${KDE4_KAUDIODEVICELIST_LIBS})

set_target_properties(kaudiodevicelist PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS kaudiodevicelist ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES
        kaudiodevicelist_export.h
        audiodevice.h
        audiodeviceenumerator.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/phonon COMPONENT Devel)

install(FILES hardwaredatabase DESTINATION ${DATA_INSTALL_DIR}/libphonon)
