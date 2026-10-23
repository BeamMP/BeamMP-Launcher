find_program(APPIMAGETOOL_PATH appimagetool OPTIONAL)

if (NOT APPIMAGETOOL_PATH)
	function(generate_appimage TARGET DESKTOP ICON)
		message("appimagetool was not found. Skipping AppImage generation")
	endfunction()
else()
	function(generate_appimage TARGET NAME DESKTOP ICON)
		cmake_path(GET ICON FILENAME ICON_FILENAME)
		cmake_path(GET DESKTOP FILENAME DESKTOP_FILENAME)
		cmake_path(GET ICON STEM ICON_FILENAME_NOEXT)

		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/bin/${NAME}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/bin
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/bin/${NAME}
			DEPENDS ${TARGET})
		
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/share/icons/hicolor/256x256/apps/${ICON_FILENAME}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/share/icons/hicolor/256x256
			COMMAND ${CMAKE_COMMAND} -E copy ${ICON} ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/share/icons/hicolor/256x256/apps/${ICON_FILENAME}
			DEPENDS ${ICON})
	
		configure_file(${DESKTOP} ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/share/applications/${DESKTOP_FILENAME})

		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/appdir/.DirIcon
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/appdir
			COMMAND ${CMAKE_COMMAND} -E copy usr/share/icons/hicolor/256x256/apps/${ICON_FILENAME} .DirIcon
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/appdir
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/share/icons/hicolor/256x256/apps/${ICON_FILENAME})

		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/appdir/AppRun
			COMMAND ${CMAKE_COMMAND} -E create_symlink usr/bin/${NAME} AppRun
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/appdir
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/bin/${NAME})

		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/appdir/${ICON_FILENAME}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/appdir
			COMMAND ${CMAKE_COMMAND} -E copy usr/share/icons/hicolor/256x256/apps/${ICON_FILENAME} ${ICON_FILENAME}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/appdir
			DEPENDS ${ICON})

		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/appdir/${DESKTOP_FILENAME}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/appdir
			COMMAND ${CMAKE_COMMAND} -E create_symlink usr/share/applications/${DESKTOP_FILENAME} ${DESKTOP_FILENAME}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/appdir
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/share/applications/${DESKTOP_FILENAME})

		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.AppImage
			COMMAND ${APPIMAGETOOL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/appdir ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.AppImage
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/bin/${NAME}
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/share/icons/hicolor/256x256/apps/${ICON_FILENAME}
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/share/applications/${DESKTOP_FILENAME}
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/.DirIcon
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/AppRun
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/${ICON_FILENAME}
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/appdir/${DESKTOP_FILENAME}
		)

		add_custom_target(${TARGET}_AppImage
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.AppImage)
	endfunction()
endif()
