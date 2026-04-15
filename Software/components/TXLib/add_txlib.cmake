function(tx_add_txlib)
	if(EXISTS "${CMAKE_SOURCE_DIR}/../libs/TXLib/CMakeLists.txt")
	    message(STATUS "Using local TXLib")
	    add_subdirectory(../libs/TXLib "${CMAKE_BINARY_DIR}/libs/TXLib")
		set(TXLib "${CMAKE_SOURCE_DIR}/../libs/TXLib" PARENT_SCOPE)
	else()
	    message(STATUS "Fetching TXLib from GitHub")

	    include(FetchContent)

	    FetchContent_Declare(
	        TXLib # declared name (you define it)
	        GIT_REPOSITORY https://github.com/TXWD1234/TXLib.git # url of the targeting repo
	        GIT_TAG main # set the version of the targeting repo
	    )

	    FetchContent_MakeAvailable(TXLib) # git clone + add_subdirector
		set(TXLib "${txlib_SOURCE_DIR}" PARENT_SCOPE)
	endif()
endfunction()

function(tx_add_txlib_version libVersion)
	if(EXISTS "../libs/TXLib/CMakeLists.txt")
	    message(STATUS "Using local TXLib")
	    add_subdirectory(../libs/TXLib "${CMAKE_BINARY_DIR}/libs/TXLib")
	else()
	    message(STATUS "Fetching TXLib from GitHub")

	    include(FetchContent)

	    FetchContent_Declare(
	        TXLib # declared name (you define it)
	        GIT_REPOSITORY https://github.com/TXWD1234/TXLib.git # url of the targeting repo
	        GIT_TAG ${libVersion} # set the version of the targeting repo
	    )

	    FetchContent_MakeAvailable(TXLib) # git clone + add_subdirector
		set(TXLib "${txlib_SOURCE_DIR}" PARENT_SCOPE)
	endif()
endfunction()

function(tx_add_txlib_components)
	if(EXISTS "${CMAKE_SOURCE_DIR}/../libs/TXLib/CMakeLists.txt")
	    message(STATUS "Using local TXLib")
	    add_subdirectory(../libs/TXLib "${CMAKE_BINARY_DIR}/libs/TXLib")
		set(TXLib "${CMAKE_SOURCE_DIR}/../libs/TXLib" PARENT_SCOPE)
	else()
	    message(STATUS "Fetching TXLib from GitHub")

		set(options "")
    	set(oneValueArgs "")
    	set(multiValueArgs COMPONENTS)
    	cmake_parse_arguments(TX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	    include(FetchContent)

	    FetchContent_Declare(
	        TXLib
	        GIT_REPOSITORY "https://github.com/TXWD1234/TXLib.git"
	        GIT_TAG "main"
			SOURCE_SUBDIR "TXCMakeUtils"
	    )
		# SOURCE_SUBDIR to work around the MakeAvailable

    	FetchContent_MakeAvailable(TXLib)
		FetchContent_GetProperties(TXLib)

    	foreach(comp IN LISTS TX_COMPONENTS)
    	    if(EXISTS "${txlib_SOURCE_DIR}/${comp}/CMakeLists.txt")
    	        message(STATUS "TXLib: Adding component [${comp}]")
    	        add_subdirectory("${txlib_SOURCE_DIR}/${comp}" "${txlib_BINARY_DIR}/${comp}")
    	    else()
    	        message(WARNING "TXLib: Component [${comp}] not found in source!")
    	    endif()
    	endforeach()

		set(TXLib "${txlib_SOURCE_DIR}" PARENT_SCOPE)
	endif()
endfunction()