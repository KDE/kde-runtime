set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory(guitest)

MACRO(PHONON_UNIT_TESTS)
	FOREACH(_testname ${ARGN})
		kde4_add_unit_test(${_testname} ${_testname}.cpp)
		target_link_libraries(${_testname} ${KDE4_PHONON_LIBS} ${QT_QTTEST_LIBRARY})
	ENDFOREACH(_testname)
ENDMACRO(PHONON_UNIT_TESTS)

# same as above but using the fake backend
MACRO(PHONON_UNIT_TESTS_FAKE)
	FOREACH(_testname ${ARGN})
		kde4_add_unit_test(${_testname}_fake ${_testname}.cpp)
      target_link_libraries(${_testname}_fake ${KDE4_PHONON_LIBS} ${QT_QTTEST_LIBRARY} phonon_fake)
		SET_TARGET_PROPERTIES(${_testname}_fake PROPERTIES COMPILE_FLAGS -DUSE_FAKE_BACKEND)
	ENDFOREACH(_testname)
ENDMACRO(PHONON_UNIT_TESTS_FAKE)

MACRO(PHONON_EXECUTABLE_TESTS)
	FOREACH(_testname ${ARGN})
		kde4_add_executable(${_testname} TEST ${_testname}.cpp)
		target_link_libraries(${_testname} ${KDE4_PHONON_LIBS} ${QT_QTTEST_LIBRARY})
	ENDFOREACH(_testname)
ENDMACRO(PHONON_EXECUTABLE_TESTS)

PHONON_EXECUTABLE_TESTS(
	mediaplayer
	)

PHONON_UNIT_TESTS(
	audiodevicelisttest
	)

PHONON_UNIT_TESTS_FAKE(
)

target_link_libraries(audiodevicelisttest ${KDE4_KAUDIODEVICELIST_LIBS})
target_link_libraries(mediaplayer ${KDE4_KIO_LIBS})

SET(backendtester_SRCS backendtester.cpp)
kde4_add_ui_files(backendtester_SRCS backendtester.ui)
kde4_add_executable(backendtester TEST ${backendtester_SRCS})
target_link_libraries(backendtester ${KDE4_KDEUI_LIBS})
