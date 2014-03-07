project(nepomuk)

find_package(NepomukCore)
set_package_properties(NepomukCore PROPERTIES DESCRIPTION "Nepomuk Core Libraries"
                       URL "https://projects.kde.org/nepomuk-core"
                       TYPE OPTIONAL
                      )

# disable deprecated libnepomuk stuff for less compiler warnings
add_definitions(-DDISABLE_NEPOMUK_LEGACY=1)

find_package(Soprano REQUIRED)
include(SopranoAddOntology)

include_directories(
  ${SOPRANO_INCLUDE_DIR}
  ${NEPOMUK_CORE_INCLUDE_DIR}
)

add_subdirectory(kcm)
add_subdirectory(kioslaves)

