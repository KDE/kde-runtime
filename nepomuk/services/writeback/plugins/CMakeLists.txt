macro_optional_find_package(Taglib)
macro_log_feature(TAGLIB_FOUND "Taglib" "For Audio Writebackplugin" "taglib" FALSE "" "Required for audio writeback of metadata.")

macro_optional_find_package(Kexiv2)
macro_log_feature(KEXIV2_FOUND "Kexiv2" "For Exif WritebackPlugin" "kexiv2" FALSE "" "Required for image writeback of metadata.")

macro_optional_find_package(KdepimLibs)
macro_log_feature(KDEPIMLIBS_FOUND "KdepimLibs" "For Akonadi WritebackPlugin" "kdepimLibs" FALSE "" "Required for akonadi writeback of metadata.")

if(TAGLIB_FOUND)
add_subdirectory(taglib)
endif()

if(KEXIV2_FOUND)
add_subdirectory(exiv2)
endif()

if(KDEPIMLIBS_FOUND)
add_subdirectory(akonadi)
endif()
