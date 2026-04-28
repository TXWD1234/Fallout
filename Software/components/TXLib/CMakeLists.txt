# Copyright (c) 2025 TXLib. Licensed under the MIT License.

if(NOT CMAKE_SCRIPT_MODE_FILE)
	include("${CMAKE_CURRENT_SOURCE_DIR}/add_txlib.cmake")
	tx_add_txlib(
		LOCAL_DIR
			/home/TX_Jerry/Programming/dev/libs/TXLib
		COMPONENTS
			TXMath
			TXData
			TXUtility
	)
endif()

idf_component_register()

if(NOT CMAKE_SCRIPT_MODE_FILE)
	target_link_libraries(${COMPONENT_LIB} INTERFACE
		TXMath
		TXData
		TXUtility
	)
	message(STATUS "Done adding TXLib.")
endif()