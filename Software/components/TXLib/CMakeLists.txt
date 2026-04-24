if(NOT CMAKE_SCRIPT_MODE_FILE)
	include("${CMAKE_CURRENT_SOURCE_DIR}/add_txlib.cmake")
	tx_add_txlib_components(COMPONENTS
		TXMath
		TXUtility
	)
endif()

idf_component_register()

if(NOT CMAKE_SCRIPT_MODE_FILE)
	target_link_libraries(${COMPONENT_LIB} INTERFACE
		TXMath
		TXUtility
	)
	message(STATUS "Done adding TXLib.")
endif()